Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/Barter.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("interfaces/DamageReceiver.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Gate.js");
Engine.LoadComponentScript("interfaces/Heal.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Pack.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/RallyPoint.js");
Engine.LoadComponentScript("interfaces/ResourceDropsite.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/Trader.js")
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("GuiInterface.js");

var cmp = ConstructComponent(SYSTEM_ENTITY, "GuiInterface");


AddMock(SYSTEM_ENTITY, IID_Barter, {
	GetPrices: function() { return {
		"buy": { "food": 150 },
		"sell": { "food": 25 },
	}},
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	GetNumPlayers: function() { return 2; },
	GetPlayerByID: function(id) { TS_ASSERT(id === 0 || id === 1); return 100+id; },
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	GetLosVisibility: function(ent, player) { return "visible"; },
	GetLosCircular: function() { return false; },
});

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	GetCurrentTemplateName: function(ent) { return "example"; },
	GetTemplate: function(name) { return ""; },
});

AddMock(SYSTEM_ENTITY, IID_Timer, {
	GetTime: function() { return 0; },
	SetTimeout: function(ent, iid, funcname, time, data) { return 0; },
});

AddMock(100, IID_Player, {
	GetName: function() { return "Player 1"; },
	GetCiv: function() { return "gaia"; },
	GetColour: function() { return { r: 1, g: 1, b: 1, a: 1}; },
	GetPopulationCount: function() { return 10; },
	GetPopulationLimit: function() { return 20; },
	GetMaxPopulation: function() { return 200; },
	GetResourceCounts: function() { return { food: 100 }; },
	IsTrainingBlocked: function() { return false; },
	GetState: function() { return "active"; },
	GetTeam: function() { return -1; },
	GetLockTeams: function() { return false; },
	GetDiplomacy: function() { return [-1, 1]; },
	GetConquestCriticalEntitiesCount: function() { return 1; },
	IsAlly: function() { return false; },
	IsNeutral: function() { return false; },
	IsEnemy: function() { return true; },
});

AddMock(100, IID_EntityLimits, {
	GetLimits: function() { return {"Foo": 10}; },
	GetCounts: function() { return {"Foo": 5}; },
});

AddMock(100, IID_TechnologyManager, {
	IsTechnologyResearched: function(tech) { return false; },
	GetTechModifications: function() { return {}; },
});

AddMock(100, IID_StatisticsTracker, {
	GetStatistics: function() { 
		return {
			"unitsTrained": 10,
			"unitsLost": 9,
			"buildingsConstructed": 5,
			"buildingsLost": 4,
			"civCentresBuilt": 1,
			"resourcesGathered": {
				"food": 100,	
				"wood": 0,	
				"metal": 0,	
				"stone": 0,
				"vegetarianFood": 0, 
			},
			"treasuresCollected": 0,
			"percentMapExplored": 10,
		};
	},
	IncreaseTrainedUnitsCounter: function() { return 1; },
	IncreaseConstructedBuildingsCounter: function() { return 1; },
	IncreaseBuiltCivCentresCounter: function() { return 1; },
});

AddMock(101, IID_Player, {
	GetName: function() { return "Player 2"; },
	GetCiv: function() { return "celt"; },
	GetColour: function() { return { r: 1, g: 0, b: 0, a: 1}; },
	GetPopulationCount: function() { return 40; },
	GetPopulationLimit: function() { return 30; },
	GetMaxPopulation: function() { return 300; },
	GetResourceCounts: function() { return { food: 200 }; },
	IsTrainingBlocked: function() { return false; },
	GetState: function() { return "active"; },
	GetTeam: function() { return -1; },
	GetLockTeams: function() {return false; },
	GetDiplomacy: function() { return [-1, 1]; },
	GetConquestCriticalEntitiesCount: function() { return 1; },
	IsAlly: function() { return true; },
	IsNeutral: function() { return false; },
	IsEnemy: function() { return false; },
});

AddMock(101, IID_EntityLimits, {
	GetLimits: function() { return {"Bar": 20}; },
	GetCounts: function() { return {"Bar": 0}; },
});

AddMock(101, IID_TechnologyManager, {
	IsTechnologyResearched: function(tech) { if (tech == "phase_village") return true; else return false; },
	GetTechModifications: function() { return {}; },
});

AddMock(101, IID_StatisticsTracker, {
	GetStatistics: function() { 
		return {
			"unitsTrained": 10,
			"unitsLost": 9,
			"buildingsConstructed": 5,
			"buildingsLost": 4,
			"civCentresBuilt": 1,
			"resourcesGathered": {
				"food": 100,	
				"wood": 0,	
				"metal": 0,	
				"stone": 0,
				"vegetarianFood": 0, 
			},
			"treasuresCollected": 0,
			"percentMapExplored": 10,
		};
	},
	IncreaseTrainedUnitsCounter: function() { return 1; },
	IncreaseConstructedBuildingsCounter: function() { return 1; },
	IncreaseBuiltCivCentresCounter: function() { return 1; },
});

// Note: property order matters when using TS_ASSERT_UNEVAL_EQUALS,
//	because uneval preserves property order. So make sure this object
//	matches the ordering in GuiInterface.
TS_ASSERT_UNEVAL_EQUALS(cmp.GetSimulationState(), {
	players: [
		{
			name: "Player 1",
			civ: "gaia",
			colour: { r:1, g:1, b:1, a:1 },
			popCount: 10,
			popLimit: 20,
			popMax: 200,
			resourceCounts: { food: 100 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			phase: "",
			isAlly: [false, false],
			isNeutral: [false, false],
			isEnemy: [true, true],
			entityLimits: {"Foo": 10},
			entityCounts: {"Foo": 5},
			techModifications: {},
		},
		{
			name: "Player 2",
			civ: "celt",
			colour: { r:1, g:0, b:0, a:1 },
			popCount: 40,
			popLimit: 30,
			popMax: 300,
			resourceCounts: { food: 200 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			phase: "village",
			isAlly: [true, true],
			isNeutral: [false, false],
			isEnemy: [false, false],
			entityLimits: {"Bar": 20},
			entityCounts: {"Bar": 0},
			techModifications: {},
		}
	],
	circularMap: false,
	timeElapsed: 0,
});

TS_ASSERT_UNEVAL_EQUALS(cmp.GetExtendedSimulationState(), {
	players: [
		{
			name: "Player 1",
			civ: "gaia",
			colour: { r:1, g:1, b:1, a:1 },
			popCount: 10,
			popLimit: 20,
			popMax: 200,
			resourceCounts: { food: 100 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			phase: "",
			isAlly: [false, false],
			isNeutral: [false, false],
			isEnemy: [true, true],
			entityLimits: {"Foo": 10},
			entityCounts: {"Foo": 5},
			techModifications: {},
			statistics: {
				unitsTrained: 10,
				unitsLost: 9,
				buildingsConstructed: 5,
				buildingsLost: 4,
				civCentresBuilt: 1,
				resourcesGathered: {
					food: 100,	
					wood: 0,	
					metal: 0,	
					stone: 0,
					vegetarianFood: 0, 
				},
				treasuresCollected: 0,
				percentMapExplored: 10,
			},
		},
		{
			name: "Player 2",
			civ: "celt",
			colour: { r:1, g:0, b:0, a:1 },
			popCount: 40,
			popLimit: 30,
			popMax: 300,
			resourceCounts: { food: 200 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			phase: "village",
			isAlly: [true, true],
			isNeutral: [false, false],
			isEnemy: [false, false],
			entityLimits: {"Bar": 20},
			entityCounts: {"Bar": 0},
			techModifications: {},
			statistics: {
				unitsTrained: 10,
				unitsLost: 9,
				buildingsConstructed: 5,
				buildingsLost: 4,
				civCentresBuilt: 1,
				resourcesGathered: {
					food: 100,	
					wood: 0,	
					metal: 0,	
					stone: 0,
					vegetarianFood: 0, 
				},
				treasuresCollected: 0,
				percentMapExplored: 10,
			},
		}
	],
	circularMap: false,
	timeElapsed: 0,
});


AddMock(10, IID_Builder, {
	GetEntitiesList: function() {
		return ["test1", "test2"];
	},
});

AddMock(10, IID_Health, {
	GetHitpoints: function() { return 50; },
	GetMaxHitpoints: function() { return 60; },
	IsRepairable: function() { return false; },
	IsUnhealable: function() { return false; },
});

AddMock(10, IID_Identity, {
	GetClassesList: function() { return ["class1", "class2"]; },
	GetRank: function() { return "foo"; },
	GetSelectionGroupName: function() { return "Selection Group Name"; },
	HasClass: function() { return true; },
});

AddMock(10, IID_Position, {
	GetPosition: function() {
		return {x:1, y:2, z:3};
	},
	IsInWorld: function() {
		return true;
	},
});

// Note: property order matters when using TS_ASSERT_UNEVAL_EQUALS,
//	because uneval preserves property order. So make sure this object
//	matches the ordering in GuiInterface.
TS_ASSERT_UNEVAL_EQUALS(cmp.GetEntityState(-1, 10), {
	id: 10,
	template: "example",
	identity: {
		rank: "foo",
		classes: ["class1", "class2"],
		selectionGroupName: "Selection Group Name",
	},
	position: {x:1, y:2, z:3},
	hitpoints: 50,
	maxHitpoints: 60,
	needsRepair: false,
	needsHeal: true,
	buildEntities: ["test1", "test2"],
	barterMarket: {
		prices: { "buy": {"food":150}, "sell": {"food":25} },
	},
	visibility: "visible",
});
