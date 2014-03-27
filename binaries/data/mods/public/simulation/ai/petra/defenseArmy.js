var PETRA = function(m)
{

// Specialization of Armies used by the defense manager.
m.DefenseArmy = function(gameState, defManager, ownEntities, foeEntities)
{
	if (!m.Army.call(this, gameState, defManager, ownEntities, foeEntities))
		return false;

	this.watchTSMultiplicator = this.Config.Defense.armyStrengthWariness;
	this.watchDecrement = this.Config.Defense.prudence;

	this.foeSubStrength = {
		"spear" : ["Infantry", "Spear"],	//also pikemen
		"sword" : ["Infantry", "Sword"],
		"ranged" : ["Infantry", "Ranged"],
		"meleeCav" : ["Cavalry", "Melee"],
		"rangedCav" : ["Cavalry", "Ranged"],
		"Elephant" : ["Elephant"],
		"meleeSiege" : ["Siege", "Melee"],
		"rangedSiege" : ["Siege", "Ranged"]
	};
	this.ownSubStrength = {
		"spear" : ["Infantry", "Spear"],	//also pikemen
		"sword" : ["Infantry", "Sword"],
		"ranged" : ["Infantry", "Ranged"],
		"meleeCav" : ["Cavalry", "Melee"],
		"rangedCav" : ["Cavalry", "Ranged"],
		"Elephant" : ["Elephant"],
		"meleeSiege" : ["Siege", "Melee"],
		"rangedSiege" : ["Siege", "Ranged"]
	};

	this.checkDangerosity(gameState);	// might push us to 1.
	this.watchLevel = this.foeStrength * this.watchTSMultiplicator;
	
	return true;
}

m.DefenseArmy.prototype = Object.create(m.Army.prototype);

m.DefenseArmy.prototype.assignUnit = function (gameState, entID)
{
	// we'll assume this defender is ours already.
	// we'll also override any previous assignment
	
	var ent = gameState.getEntityById(entID);
	if (!ent || !ent.position())
		return false;
	
	var idMin = undefined;
	var distMin = undefined;
	var idMinAll = undefined;
	var distMinAll = undefined; 
	for each (var id in this.foeEntities)
	{
		var eEnt = gameState.getEntityById(id);
		if (!eEnt || !eEnt.position())	// probably can't happen.
			continue;

		if (eEnt.unitAIOrderData().length && eEnt.unitAIOrderData()[0]["target"] &&
			eEnt.unitAIOrderData()[0]["target"] === entID)
		{   // being attacked  >>> target the unit
			idMin = id;
			break;
		}

		var dist = API3.SquareVectorDistance(ent.position(), eEnt.position());
		if (idMinAll === undefined || dist < distMin)
		{
			idMinAll = id;
			distMinAll = dist;
		}
		if (this.assignedAgainst[id].length > 2)   // already enough units against it
			continue;
		var dist = API3.SquareVectorDistance(ent.position(), eEnt.position());
		if (idMin === undefined || dist < distMin)
		{
			idMin = id;
			distMin = dist;
		}
	}

	if (idMin !== undefined)
	{
		this.assignedTo[entID] = idMin;
		this.assignedAgainst[idMin].push(entID);
		ent.attack(idMin);
		return true;
	}
	else if (idMinAll !== undefined)
	{
		this.assignedTo[entID] = idMinAll;
		this.assignedAgainst[idMinAll].push(entID);
		ent.attack(idMinAll);
		return true;
	}

	this.recalculatePosition(gameState);
	ent.attackMove(this.foePosition[0], this.foePosition[1]);
	return false;
}

// TODO: this should return cleverer results ("needs anti-elephant"…)
m.DefenseArmy.prototype.needsDefenders = function (gameState, events)
{
	// some preliminary checks because we don't update for tech
	if (this.foeStrength < 0 || this.ownStrength < 0)
		this.recalculateStrengths(gameState);
	
	if (this.foeStrength * this.defenseRatio <= this.ownStrength)
		return false;
	return this.foeStrength * this.defenseRatio - this.ownStrength;
}

m.DefenseArmy.prototype.getState = function (gameState)
{
	if (this.foeEntities.length === 0)
		return 0;
	if (this.state === 2)
		return 2;
	if (this.watchLevel > 0)
		return 1;
	return 0;
}

// check if we should remain at state 2 or drift away
m.DefenseArmy.prototype.checkDangerosity = function (gameState)
{
	// right now we'll check if our position is "enemy" or not.
	if (gameState.ai.HQ.territoryMap.getOwner(this.ownPosition) !== PlayerID)
		this.state = 1;
	else if (this.state === 1)
		this.state = 2;
}

m.DefenseArmy.prototype.update = function (gameState)
{
	var breakaways = this.onUpdate(gameState);

	this.checkDangerosity(gameState);
	
	var normalWatch = this.foeStrength * this.watchTSMultiplicator;
	if (this.state === 2)
		this.watchLevel = normalWatch;
	else if (this.watchLevel > normalWatch)
		this.watchLevel = normalWatch;
	else
		this.watchLevel -= this.watchDecrement;
	
	// TODO: deal with watchLevel?
	
	return breakaways;
}

m.DefenseArmy.prototype.debug = function (gameState)
{
	m.debug(" ");
	m.debug ("Army " + this.ID)
//	m.debug ("state " + this.state);
//	m.debug ("WatchLevel " + this.watchLevel);
//	m.debug ("Entities " + this.foeEntities.length);
//	m.debug ("Strength " + this.foeStrength);
	//	debug (gameState.getEntityById(ent)._templateName + ", ID " + ent);
	//debug ("Defenders " + this.ownEntities.length);
	for each (ent in this.foeEntities)
	{
		if (gameState.getEntityById(ent) !== undefined)
		{
			m.debug (gameState.getEntityById(ent)._templateName + ", ID " + ent);
		Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent], "rgb": [0.5,0,0]});
		} else
			m.debug("ent "  + ent);
	}
	m.debug ("");
	
}
return m;
}(PETRA);
