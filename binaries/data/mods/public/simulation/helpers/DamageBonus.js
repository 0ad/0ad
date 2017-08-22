/**
 *  Calculate the attack damage multiplier against a target.
 */
function GetDamageBonus(target, template)
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
		if (bonus.Classes && bonus.Classes.split(/\s+/).some(cls => !cmpIdentity.HasClass(cls)))
			continue;
		attackBonus *= bonus.Multiplier;
	}

	return attackBonus;
}

Engine.RegisterGlobal("GetDamageBonus", GetDamageBonus);
