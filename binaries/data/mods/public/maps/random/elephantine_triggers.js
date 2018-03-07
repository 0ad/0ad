Trigger.prototype.InitElephantine = function()
{
	this.InitElephantine_DefenderStance();
	this.InitElephantine_GarrisonBuildings();
};

Trigger.prototype.InitElephantine_DefenderStance = function()
{
	for (let ent of Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(0))
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (cmpIdentity && cmpIdentity.HasClass("Soldier"))
			TriggerHelper.SetUnitStance(ent, "defensive");
	}
};

Trigger.prototype.InitElephantine_GarrisonBuildings = function()
{
	let kushInfantryUnits = TriggerHelper.GetTemplateNamesByClasses("CitizenSoldier+Infantry", "kush", undefined, "Elite", true);
	let kushSupportUnits = TriggerHelper.GetTemplateNamesByClasses("FemaleCitizen Healer", "kush", undefined, "Elite", true);

	TriggerHelper.SpawnAndGarrisonAtClasses(0, "Tower", kushInfantryUnits, 1);

	for (let identityClass of ["Wonder", "Temple", "Pyramid"])
		TriggerHelper.SpawnAndGarrisonAtClasses(0, identityClass, kushInfantryUnits.concat(kushSupportUnits), 1);
};

{
	Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).RegisterTrigger("OnInitGame", "InitElephantine", { "enabled": true });
}
