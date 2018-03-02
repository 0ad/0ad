Trigger.prototype.InitElephantine = function()
{
	for (let ent of Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(0))
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (cmpIdentity && cmpIdentity.HasClass("Soldier"))
			Engine.QueryInterface(ent, IID_UnitAI).SwitchToStance("defensive");
	}

	let kushSupportUnits = [
		"units/kush_support_healer_e",
		"units/kush_support_female_citizen"
	];

	let kushInfantryUnits = [
		"units/kush_infantry_archer_e",
		"units/kush_infantry_spearman_e"
	];

	this.SpawnAndGarrison(0, "Tower", kushInfantryUnits, 1);

	for (let identityClass of ["Wonder", "Temple", "Pyramid"])
		this.SpawnAndGarrison(0, identityClass, kushInfantryUnits.concat(kushSupportUnits), 1);
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.RegisterTrigger("OnInitGame", "InitElephantine", { "enabled": true });
}
