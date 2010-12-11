function BuildRestrictions() {}

BuildRestrictions.prototype.Schema =
	"<element name='PlacementType'>" +
		"<choice>" +
			"<value>standard</value>" +
			"<value>settlement</value>" +
		"</choice>" + // TODO: add special types for fields, docks, walls
	"</element>" +
	"<element name='Territory'>" +
		"<choice>" +
			"<value>all</value>" +
			"<value>allied</value>" +
		"</choice>" +
	"</element>" +
	"<element name='Category'>" +
		"<choice>" +
			"<value>CivilCentre</value>" +
			"<value>House</value>" +
			"<value>ScoutTower</value>" +
			"<value>Farmstead</value>" +
			"<value>Market</value>" +
			"<value>Barracks</value>" +
			"<value>Dock</value>" +
			"<value>Fortress</value>" +
			"<value>Field</value>" +
			"<value>Temple</value>" +
			"<value>Wall</value>" +
			"<value>Fence</value>" +
			"<value>Mill</value>" +
			"<value>Stoa</value>" +
			"<value>Resource</value>" +
			"<value>Special</value>" +
		"</choice>" +
	"</element>";
	// TODO: add phases, prerequisites, etc

/*
 * TODO: the vague plan for Category is to add some BuildLimitManager which
 * specifies the limit per category, and which can determine whether you're
 * allowed to build more (based on the number in total / per territory / per
 * civ center as appropriate)
 */

/*
 * TODO: the vague plan for PlacementType is that it may restrict the locations
 * and orientations of new construction work (e.g. civ centers must be on settlements,
 * docks must be on shores), which affects the UI and the build permissions
 */

BuildRestrictions.prototype.OnOwnershipChanged = function(msg)
{
	if (this.template.Category)
	{
		var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		if (msg.from != -1)
		{
			var fromPlayerBuildLimits = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(msg.from), IID_BuildLimits);
			fromPlayerBuildLimits.DecrementCount(this.template.Category);
		}
		if (msg.to != -1)
		{
			var toPlayerBuildLimits = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(msg.to), IID_BuildLimits);
			toPlayerBuildLimits.IncrementCount(this.template.Category);	
		}
	}
};

BuildRestrictions.prototype.GetCategory = function()
{
	return this.template.Category;
};

Engine.RegisterComponentType(IID_BuildRestrictions, "BuildRestrictions", BuildRestrictions);
