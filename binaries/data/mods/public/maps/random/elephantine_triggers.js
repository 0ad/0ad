Trigger.prototype.InitElephantine = function()
{
	let gaiaEnts = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(0);

	for (let ent of gaiaEnts)
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (cmpIdentity && cmpIdentity.HasClass("Soldier"))
			Engine.QueryInterface(ent, IID_UnitAI).SwitchToStance("defensive");
	}

	let kushSupportUnits = ["kush_support_healer_e", "kush_support_female_citizen"];
	let kushInfantryUnits = ["kush_infantry_archer_e", "kush_infantry_spearman_e"];

	this.SpawnAndGarrisonBuilding(gaiaEnts, "Tower", kushInfantryUnits);

	for (let identityClass of ["Wonder", "Temple", "Pyramid"])
		this.SpawnAndGarrisonBuilding(gaiaEnts, identityClass, kushInfantryUnits.concat(kushSupportUnits));
};

// Shameless copy of Danubius

Trigger.prototype.SpawnAndGarrisonBuilding = function(gaiaEnts, targetClass, templates)
{
	for (let gaiaEnt of gaiaEnts)
	{
		let cmpIdentity = Engine.QueryInterface(gaiaEnt, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass(targetClass))
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(gaiaEnt, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		let unitCounts = this.RandomAttackerTemplates(templates, cmpGarrisonHolder.GetCapacity());

		for (let template in unitCounts)
			for (let newEnt of TriggerHelper.SpawnUnits(gaiaEnt, "units/" + template, unitCounts[template], 0))
				Engine.QueryInterface(gaiaEnt, IID_GarrisonHolder).Garrison(newEnt);
	}
};

/**
 * Return a random amount of these templates whose sum is count.
 */
Trigger.prototype.RandomAttackerTemplates = function(templates, count)
{
	let ratios = new Array(templates.length).fill(1).map(i => randFloat(0, 1));
	let ratioSum = ratios.reduce((current, sum) => current + sum, 0);

	let remainder = count;
	let templateCounts = {};

	for (let i in templates)
	{
		let currentCount = +i == templates.length - 1 ? remainder : Math.round(ratios[i] / ratioSum * count);
		if (!currentCount)
			continue;

		templateCounts[templates[i]] = currentCount;
		remainder -= currentCount;
	}

	if (remainder != 0)
		warn("Not as many templates as expected: " + count + " vs " + uneval(templateCounts));

	return templateCounts;
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.RegisterTrigger("OnInitGame", "InitElephantine", { "enabled": true });
}
