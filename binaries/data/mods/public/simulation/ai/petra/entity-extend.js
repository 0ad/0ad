var PETRA = function(m)
{

// returns some sort of DPS * health factor. If you specify a class, it'll use the modifiers against that class too.
m.getMaxStrength = function(ent, againstClass)
{
	var strength = 0.0;
	var attackTypes = ent.attackTypes();
	var armourStrength = ent.armourStrengths();
	var hp = ent.maxHitpoints() / 100.0;	// some normalization
	for (var typeKey in attackTypes) {
		var type = attackTypes[typeKey];
		
		if (type == "Slaughter" || type == "Charged")
			continue;
		
		var attackStrength = ent.attackStrengths(type);
		var attackRange = ent.attackRange(type);
		var attackTimes = ent.attackTimes(type);
		for (var str in attackStrength) {
			var val = parseFloat(attackStrength[str]);
			if (againstClass)
				val *= ent.getMultiplierAgainst(type, againstClass);
			switch (str) {
				case "crush":
					strength += (val * 0.085) / 3;
					break;
				case "hack":
					strength += (val * 0.075) / 3;
					break;
				case "pierce":
					strength += (val * 0.065) / 3;
					break;
			}
		}
		if (attackRange){
			strength += (attackRange.max * 0.0125) ;
		}
		for (var str in attackTimes) {
			var val = parseFloat(attackTimes[str]);
			switch (str){
				case "repeat":
					strength += (val / 100000);
					break;
				case "prepare":
					strength -= (val / 100000);
					break;
			}
		}
	}
	for (var str in armourStrength) {
		var val = parseFloat(armourStrength[str]);
		switch (str) {
			case "crush":
				strength += (val * 0.085) / 3;
				break;
			case "hack":
				strength += (val * 0.075) / 3;
				break;
			case "pierce":
				strength += (val * 0.065) / 3;
				break;
		}
	}
	return strength * hp;
};

// Makes the worker deposit the currently carried resources at the closest accessible dropsite
m.returnResources = function(ent, gameState)
{
	if (!ent.resourceCarrying() || ent.resourceCarrying().length == 0 || !ent.position())
		return false;

	var resource = ent.resourceCarrying()[0].type;

	var closestDropsite = undefined;
	var distmin = Math.min();
	var access = gameState.ai.accessibility.getAccessValue(ent.position());
	gameState.getOwnDropsites(resource).forEach(function(dropsite) {
		if (!dropsite.position() || dropsite.getMetadata(PlayerID, "access") !== access)
			return;
		var dist = API3.SquareVectorDistance(ent.position(), dropsite.position());
		if (dist > distmin)
			return;
		distmin = dist;
		closestDropsite = dropsite;
	});
	
	if (!closestDropsite)
		return false;	
	ent.returnResources(closestDropsite);
	return true;
};

m.getHolder = function(ent, gameState)
{
	var found = undefined;
	gameState.getEntities().forEach(function (holder) {
		if (found || !holder.isGarrisonHolder())
			return;
		if (holder._entity.garrisoned.indexOf(ent.id()) !== -1)
			found = holder;
	});
	return found;
};

m.dumpEntity = function(ent)
{
	if (!ent)
		return;
	API3.warn(" >>> id " + ent.id() + " name " + ent.genericName() + " pos " + ent.position()
		+ " state " + ent.unitAIState());
	API3.warn(" base " + ent.getMetadata(PlayerID, "base") + " >>> role " + ent.getMetadata(PlayerID, "role")
		+ " subrole " + ent.getMetadata(PlayerID, "subrole"));
	API3.warn(" garrisoning " + ent.getMetadata(PlayerID, "garrisoning") + " garrisonHolder " + ent.getMetadata(PlayerID, "garrisonHolder")
		+ " plan " + ent.getMetadata(PlayerID, "plan")	+ " transport " + ent.getMetadata(PlayerID, "transport")
		+ " gather-type " + ent.getMetadata(PlayerID, "gather-type") + " target-foundation " + ent.getMetadata(PlayerID, "target-foundation")
		+ " PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
};

return m;
}(PETRA);
