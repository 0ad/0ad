var PETRA = function(m)
{

// Specialization of Armies used by the defense manager.
m.DefenseArmy = function(gameState, defManager, ownEntities, foeEntities)
{
	if (!m.Army.call(this, gameState, defManager, ownEntities, foeEntities))
		return false;

	return true;
};

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
		if (idMinAll === undefined || dist < distMinAll)
		{
			idMinAll = id;
			distMinAll = dist;
		}
		if (this.assignedAgainst[id].length > 2)   // already enough units against it
			continue;
		if (idMin === undefined || dist < distMin)
		{
			idMin = id;
			distMin = dist;
		}
	}

	if (idMin !== undefined)
		var idFoe = idMin;
	else if (idMinAll !== undefined)
		var idFoe = idMinAll
	else
		return false;

	var ownIndex = gameState.ai.accessibility.getAccessValue(ent.position());
	var foePosition = gameState.getEntityById(idFoe).position();
	var foeIndex = gameState.ai.accessibility.getAccessValue(foePosition);
	if (ownIndex === foeIndex || ent.hasClass("Ship"))
	{
		this.assignedTo[entID] = idFoe;
		this.assignedAgainst[idFoe].push(entID);
		ent.attack(idFoe);
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, ent, ownIndex, foeIndex, foePosition);
	return true;
};

// TODO: this should return cleverer results ("needs anti-elephant"…)
m.DefenseArmy.prototype.needsDefenders = function (gameState, events)
{
	// some preliminary checks because we don't update for tech
	if (this.foeStrength < 0 || this.ownStrength < 0)
		this.recalculateStrengths(gameState);
	
	if (this.foeStrength * this.defenseRatio <= this.ownStrength)
		return false;
	return this.foeStrength * this.defenseRatio - this.ownStrength;
};

m.DefenseArmy.prototype.getState = function (gameState)
{
	if (this.foeEntities.length === 0)
		return 0;
	return 1;
};

m.DefenseArmy.prototype.update = function (gameState)
{
	for each (var entId in this.ownEntities)
	{
		var ent = gameState.getEntityById(entId);
		if (!ent)
			continue;
		var orders = ent.unitAIOrderData();
		if (orders.length === 0 && !ent.getMetadata(PlayerID, "transport"))
			this.assignUnit(gameState, entId);
	}

	var breakaways = this.onUpdate(gameState);

	return breakaways;
};

m.DefenseArmy.prototype.debug = function (gameState)
{
	m.debug(" ");
	m.debug ("Army " + this.ID)
//	m.debug ("Entities " + this.foeEntities.length);
//	m.debug ("Strength " + this.foeStrength);
	//	debug (gameState.getEntityById(ent)._templateName + ", ID " + ent);
	//debug ("Defenders " + this.ownEntities.length);
	for each (ent in this.foeEntities)
	{
		if (gameState.getEntityById(ent) !== undefined)
		{
			warn(gameState.getEntityById(ent)._templateName + ", ID " + ent);
			Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent], "rgb": [0.5,0,0]});
		}
		else
			warn("ent "  + ent);
	}
	m.debug ("");
};

return m;
}(PETRA);
