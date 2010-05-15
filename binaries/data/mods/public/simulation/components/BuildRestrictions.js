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
	"<optional>" +
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
				"<value>Special</value>" +
			"</choice>" +
		"</element>" +
	"</optional>";
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

Engine.RegisterComponentType(IID_BuildRestrictions, "BuildRestrictions", BuildRestrictions);
