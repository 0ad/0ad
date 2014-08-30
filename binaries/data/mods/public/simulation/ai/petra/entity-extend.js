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
	API3.warn(" >>> pos " + ent.position() + " state " + ent.unitAIState());
	API3.warn(" >>> role " + ent.getMetadata(PlayerID, "role") + " subrole " + ent.getMetadata(PlayerID, "subrole")
		+ " garrisoning " + ent.getMetadata(PlayerID, "garrisoning") + " garrisonHolder " + ent.getMetadata(PlayerID, "garrisonHolder")
		+ " plan " + ent.getMetadata(PlayerID, "plan")	+ " transport " + ent.getMetadata(PlayerID, "transport")
		+ " gather-type " + ent.getMetadata(PlayerID, "gather-type") + " target-foundation " + ent.getMetadata(PlayerID, "target-foundation")
		+ " PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
};

return m;
}(PETRA);
