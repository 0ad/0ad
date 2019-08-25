/**
 * Calculates the attack damage multiplier against a target.
 * @param {entity_id_t} source - The source entity's id.
 * @param {entity_id_t} target - The target entity's id.
 * @param {string} type - The type of attack.
 * @param {Object} template - The bonus' template.
 * @return {number} - The source entity's attack bonus against the specified target.
 */
function GetAttackBonus(source, target, type, template)
{
	let attackBonus = 1;

	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return 1;

	// Multiply the bonuses for all matching classes
	for (let key in template)
	{
		let bonus = template[key];
		if (bonus.Civ && bonus.Civ !== cmpIdentity.GetCiv())
			continue;
		if (!bonus.Classes || MatchesClassList(cmpIdentity.GetClassesList(), bonus.Classes))
			attackBonus *= ApplyValueModificationsToEntity("Attack/" + type + "/Bonuses/" + key + "/Multiplier", +bonus.Multiplier, source);
	}

	return attackBonus;
}

Engine.RegisterGlobal("GetAttackBonus", GetAttackBonus);
