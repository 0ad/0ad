var PETRA = function(m)
{

// Specialization of Armies used by the defense manager.
m.DefenseArmy = function(gameState, ownEntities, foeEntities)
{
	if (!m.Army.call(this, gameState, ownEntities, foeEntities))
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

	// try to return its resources, and if any, the attack order will be queued
	var queued = m.returnResources(gameState, ent);

	var idMin;
	var distMin;
	var idMinAll;
	var distMinAll; 
	for (let id of this.foeEntities)
	{
		let eEnt = gameState.getEntityById(id);
		if (!eEnt || !eEnt.position())	// probably can't happen.
			continue;

		if (eEnt.hasClass("Unit") && eEnt.unitAIOrderData() && eEnt.unitAIOrderData().length && 
			eEnt.unitAIOrderData()[0]["target"] && eEnt.unitAIOrderData()[0]["target"] == entID)
		{   // being attacked  >>> target the unit
			idMin = id;
			break;
		}

		// already enough units against it
		if (this.assignedAgainst[id].length > 8
			|| (this.assignedAgainst[id].length > 5 && !eEnt.hasClass("Hero") && !eEnt.hasClass("Siege")))
			continue;

		let dist = API3.SquareVectorDistance(ent.position(), eEnt.position());
		if (idMinAll === undefined || dist < distMinAll)
		{
			idMinAll = id;
			distMinAll = dist;
		}
		if (this.assignedAgainst[id].length > 2)
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
		var idFoe = idMinAll;
	else
		return false;

	let ownIndex = gameState.ai.accessibility.getAccessValue(ent.position());
	let foeEnt = gameState.getEntityById(idFoe);
	let foePosition = foeEnt.position();
	let foeIndex = gameState.ai.accessibility.getAccessValue(foePosition);
	if (ownIndex == foeIndex || ent.hasClass("Ship"))
	{
		this.assignedTo[entID] = idFoe;
		this.assignedAgainst[idFoe].push(entID);
		ent.attack(idFoe, m.allowCapture(ent, foeEnt), queued);
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, ent, ownIndex, foeIndex, foePosition);
	return true;
};

// TODO: this should return cleverer results ("needs anti-elephant"…)
m.DefenseArmy.prototype.needsDefenders = function (gameState)
{
	// some preliminary checks because we don't update for tech so entStrength removed can be > entStrength added 
	if (this.foeStrength <= 0 || this.ownStrength <= 0)
		this.recalculateStrengths(gameState);
	
	if (this.foeStrength * this.defenseRatio <= this.ownStrength)
		return false;
	return this.foeStrength * this.defenseRatio - this.ownStrength;
};

m.DefenseArmy.prototype.getState = function ()
{
	if (this.foeEntities.length == 0)
		return 0;
	return 1;
};

m.DefenseArmy.prototype.update = function (gameState)
{
	for (let entId of this.ownEntities)
	{
		let ent = gameState.getEntityById(entId);
		if (!ent)
			continue;
		let orderData = ent.unitAIOrderData();
		if (!orderData.length && !ent.getMetadata(PlayerID, "transport"))
			this.assignUnit(gameState, entId);
		else if (orderData.length && orderData[0].target && orderData[0].attackType && orderData[0].attackType === "Capture")
		{
			let target = gameState.getEntityById(orderData[0].target);
			if (target && !m.allowCapture(ent, target))
				ent.attack(orderData[0].target, false);
		}
	}

	return this.onUpdate(gameState);
};

m.DefenseArmy.prototype.Serialize = function()
{
	return {
		"ID": this.ID,
		"foePosition": this.foePosition,
		"positionLastUpdate": this.positionLastUpdate,
		"assignedAgainst": this.assignedAgainst,
		"assignedTo": this.assignedTo,
		"foeEntities": this.foeEntities,
		"foeStrength": this.foeStrength,
		"ownEntities": this.ownEntities,
		"ownStrength": this.ownStrength,
	};
};

m.DefenseArmy.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
