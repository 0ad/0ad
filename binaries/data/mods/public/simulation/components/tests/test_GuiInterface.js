Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/AlertRaiser.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/Barter.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/CeasefireManager.js");
Engine.LoadComponentScript("interfaces/DamageReceiver.js");
Engine.LoadComponentScript("interfaces/DeathDamage.js");
Engine.LoadComponentScript("interfaces/EndGameManager.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Gate.js");
Engine.LoadComponentScript("interfaces/Guard.js");
Engine.LoadComponentScript("interfaces/Heal.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Loot.js");
Engine.LoadComponentScript("interfaces/Market.js");
Engine.LoadComponentScript("interfaces/Pack.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/Repairable.js");
Engine.LoadComponentScript("interfaces/ResourceDropsite.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/ResourceTrickle.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/Trader.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("interfaces/Upgrade.js");
Engine.LoadComponentScript("interfaces/BuildingAI.js");
Engine.LoadComponentScript("GuiInterface.js");

Resources = {
	"GetCodes": () => ["food", "metal", "stone", "wood"],
	"GetNames": () => ({
		"food": "Food",
		"metal": "Metal",
		"stone": "Stone",
		"wood": "Wood"
	}),
	"GetResource": resource => ({
		"aiAnalysisInfluenceGroup":
			resource == "food" ? "ignore" :
			resource == "wood" ? "abundant" : "sparse"
	})
};

var cmp = ConstructComponent(SYSTEM_ENTITY, "GuiInterface");


AddMock(SYSTEM_ENTITY, IID_Barter, {
	GetPrices: function() {
		return {
			"buy": { "food": 150 },
			"sell": { "food": 25 }
		};
	},
	PlayerHasMarket: function () { return false; }
});

AddMock(SYSTEM_ENTITY, IID_EndGameManager, {
	GetVictoryConditions: () => ["conquest", "wonder"],
	GetAlliedVictory: function() { return false; }
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	GetNumPlayers: function() { return 2; },
	GetPlayerByID: function(id) { TS_ASSERT(id === 0 || id === 1); return 100+id; }
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	GetLosVisibility: function(ent, player) { return "visible"; },
	GetLosCircular: function() { return false; }
});

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	GetCurrentTemplateName: function(ent) { return "example"; },
	GetTemplate: function(name) { return ""; }
});

AddMock(SYSTEM_ENTITY, IID_Timer, {
	GetTime: function() { return 0; },
	SetTimeout: function(ent, iid, funcname, time, data) { return 0; }
});

AddMock(100, IID_Player, {
	GetName: function() { return "Player 1"; },
	GetCiv: function() { return "gaia"; },
	GetColor: function() { return { r: 1, g: 1, b: 1, a: 1}; },
	CanControlAllUnits: function() { return false; },
	GetPopulationCount: function() { return 10; },
	GetPopulationLimit: function() { return 20; },
	GetMaxPopulation: function() { return 200; },
	GetResourceCounts: function() { return { food: 100 }; },
	GetPanelEntities: function() { return []; },
	IsTrainingBlocked: function() { return false; },
	GetState: function() { return "active"; },
	GetTeam: function() { return -1; },
	GetLockTeams: function() { return false; },
	GetCheatsEnabled: function() { return false; },
	GetDiplomacy: function() { return [-1, 1]; },
	IsAlly: function() { return false; },
	IsMutualAlly: function() { return false; },
	IsNeutral: function() { return false; },
	IsEnemy: function() { return true; },
	GetDisabledTemplates: function() { return {}; },
	GetDisabledTechnologies: function() { return {}; },
	GetSpyCostMultiplier: function() { return 1; },
	HasSharedDropsites: function() { return false; },
	HasSharedLos: function() { return false; }
});

AddMock(100, IID_EntityLimits, {
	GetLimits: function() { return {"Foo": 10}; },
	GetCounts: function() { return {"Foo": 5}; },
	GetLimitChangers: function() {return {"Foo": {}}; }
});

AddMock(100, IID_TechnologyManager, {
	"IsTechnologyResearched": tech => tech == "phase_village",
	"GetQueuedResearch": () => new Map(),
	"GetStartedTechs": () => new Set(),
	"GetResearchedTechs": () => new Set(),
	"GetClassCounts": () => ({}),
	"GetTypeCountsByClass": () => ({})
});

AddMock(100, IID_StatisticsTracker, {
	GetBasicStatistics: function() {
		return {
			"resourcesGathered": {
				"food": 100,
				"wood": 0,
				"metal": 0,
				"stone": 0,
				"vegetarianFood": 0
			},
			"percentMapExplored": 10
		};
	},
	GetSequences: function() {
		return {
			"unitsTrained": [0, 10],
			"unitsLost": [0, 42],
			"buildingsConstructed": [1, 3],
			"buildingsCaptured": [3, 7],
			"buildingsLost": [3, 10],
			"civCentresBuilt": [4, 10],
			"resourcesGathered": {
				"food": [5, 100],
				"wood": [0, 0],
				"metal": [0, 0],
				"stone": [0, 0],
				"vegetarianFood": [0, 0]
			},
			"treasuresCollected": [1, 20],
			"lootCollected": [0, 2],
			"percentMapExplored": [0, 10],
			"teamPercentMapExplored": [0, 10],
			"percentMapControlled": [0, 10],
			"teamPercentMapControlled": [0, 10],
			"peakPercentOfMapControlled": [0, 10],
			"teamPeakPercentOfMapControlled": [0, 10]
		};
	},
	IncreaseTrainedUnitsCounter: function() { return 1; },
	IncreaseConstructedBuildingsCounter: function() { return 1; },
	IncreaseBuiltCivCentresCounter: function() { return 1; }
});

AddMock(101, IID_Player, {
	GetName: function() { return "Player 2"; },
	GetCiv: function() { return "mace"; },
	GetColor: function() { return { r: 1, g: 0, b: 0, a: 1}; },
	CanControlAllUnits: function() { return true; },
	GetPopulationCount: function() { return 40; },
	GetPopulationLimit: function() { return 30; },
	GetMaxPopulation: function() { return 300; },
	GetResourceCounts: function() { return { food: 200 }; },
	GetPanelEntities: function() { return []; },
	IsTrainingBlocked: function() { return false; },
	GetState: function() { return "active"; },
	GetTeam: function() { return -1; },
	GetLockTeams: function() {return false; },
	GetCheatsEnabled: function() { return false; },
	GetDiplomacy: function() { return [-1, 1]; },
	IsAlly: function() { return true; },
	IsMutualAlly: function() {return false; },
	IsNeutral: function() { return false; },
	IsEnemy: function() { return false; },
	GetDisabledTemplates: function() { return {}; },
	GetDisabledTechnologies: function() { return {}; },
	GetSpyCostMultiplier: function() { return 1; },
	HasSharedDropsites: function() { return false; },
	HasSharedLos: function() { return false; }
});

AddMock(101, IID_EntityLimits, {
	GetLimits: function() { return {"Bar": 20}; },
	GetCounts: function() { return {"Bar": 0}; },
	GetLimitChangers: function() {return {"Bar": {}}; }
});

AddMock(101, IID_TechnologyManager, {
	"IsTechnologyResearched": tech => tech == "phase_village",
	"GetQueuedResearch": () => new Map(),
	"GetStartedTechs": () => new Set(),
	"GetResearchedTechs": () => new Set(),
	"GetClassCounts": () => ({}),
	"GetTypeCountsByClass": () => ({})
});

AddMock(101, IID_StatisticsTracker, {
	GetBasicStatistics: function() {
		return {
			"resourcesGathered": {
				"food": 100,
				"wood": 0,
				"metal": 0,
				"stone": 0,
				"vegetarianFood": 0
			},
			"percentMapExplored": 10
		};
	},
	GetSequences: function() {
		return {
			"unitsTrained": [0, 10],
			"unitsLost": [0, 9],
			"buildingsConstructed": [0, 5],
			"buildingsCaptured": [0, 7],
			"buildingsLost": [0, 4],
			"civCentresBuilt": [0, 1],
			"resourcesGathered": {
				"food": [0, 100],
				"wood": [0, 0],
				"metal": [0, 0],
				"stone": [0, 0],
				"vegetarianFood": [0, 0]
			},
			"treasuresCollected": [0, 0],
			"lootCollected": [0, 0],
			"percentMapExplored": [0, 10],
			"teamPercentMapExplored": [0, 10],
			"percentMapControlled": [0, 10],
			"teamPercentMapControlled": [0, 10],
			"peakPercentOfMapControlled": [0, 10],
			"teamPeakPercentOfMapControlled": [0, 10]
		};
	},
	IncreaseTrainedUnitsCounter: function() { return 1; },
	IncreaseConstructedBuildingsCounter: function() { return 1; },
	IncreaseBuiltCivCentresCounter: function() { return 1; }
});

// Note: property order matters when using TS_ASSERT_UNEVAL_EQUALS,
//	because uneval preserves property order. So make sure this object
//	matches the ordering in GuiInterface.
TS_ASSERT_UNEVAL_EQUALS(cmp.GetSimulationState(), {
	players: [
		{
			name: "Player 1",
			civ: "gaia",
			color: { r:1, g:1, b:1, a:1 },
			controlsAll: false,
			popCount: 10,
			popLimit: 20,
			popMax: 200,
			panelEntities: [],
			resourceCounts: { food: 100 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			cheatsEnabled: false,
			disabledTemplates: {},
			disabledTechnologies: {},
			hasSharedDropsites: false,
			hasSharedLos: false,
			spyCostMultiplier: 1,
			phase: "village",
			isAlly: [false, false],
			isMutualAlly: [false, false],
			isNeutral: [false, false],
			isEnemy: [true, true],
			entityLimits: {"Foo": 10},
			entityCounts: {"Foo": 5},
			entityLimitChangers: {"Foo": {}},
			researchQueued: new Map(),
			researchStarted: new Set(),
			researchedTechs: new Set(),
			classCounts: {},
			typeCountsByClass: {},
			canBarter: false,
			barterPrices: {
				"buy": { "food": 150 },
				"sell": { "food": 25 }
			},
			statistics: {
				resourcesGathered: {
					food: 100,
					wood: 0,
					metal: 0,
					stone: 0,
					vegetarianFood: 0
				},
				percentMapExplored: 10
			}
		},
		{
			name: "Player 2",
			civ: "mace",
			color: { r:1, g:0, b:0, a:1 },
			controlsAll: true,
			popCount: 40,
			popLimit: 30,
			popMax: 300,
			panelEntities: [],
			resourceCounts: { food: 200 },
			trainingBlocked: false,
			state: "active",
			team: -1,
			teamsLocked: false,
			cheatsEnabled: false,
			disabledTemplates: {},
			disabledTechnologies: {},
			hasSharedDropsites: false,
			hasSharedLos: false,
			spyCostMultiplier: 1,
			phase: "village",
			isAlly: [true, true],
			isMutualAlly: [false, false],
			isNeutral: [false, false],
			isEnemy: [false, false],
			entityLimits: {"Bar": 20},
			entityCounts: {"Bar": 0},
			entityLimitChangers: {"Bar": {}},
			researchQueued: new Map(),
			researchStarted: new Set(),
			researchedTechs: new Set(),
			classCounts: {},
			typeCountsByClass: {},
			canBarter: false,
			barterPrices: {
				"buy": { "food": 150 },
				"sell": { "food": 25 }
			},
			statistics: {
				resourcesGathered: {
					food: 100,
					wood: 0,
					metal: 0,
					stone: 0,
					vegetarianFood: 0
				},
				percentMapExplored: 10
			}
		}
	],
	circularMap: false,
	timeElapsed: 0,
	"victoryConditions": ["conquest", "wonder"],
	alliedVictory: false
});

TS_ASSERT_UNEVAL_EQUALS(cmp.GetExtendedSimulationState(), {
	"players": [
		{
			"name": "Player 1",
			"civ": "gaia",
			"color": { "r":1, "g":1, "b":1, "a":1 },
			"controlsAll": false,
			"popCount": 10,
			"popLimit": 20,
			"popMax": 200,
			"panelEntities": [],
			"resourceCounts": { "food": 100 },
			"trainingBlocked": false,
			"state": "active",
			"team": -1,
			"teamsLocked": false,
			"cheatsEnabled": false,
			"disabledTemplates": {},
			"disabledTechnologies": {},
			"hasSharedDropsites": false,
			"hasSharedLos": false,
			"spyCostMultiplier": 1,
			"phase": "village",
			"isAlly": [false, false],
			"isMutualAlly": [false, false],
			"isNeutral": [false, false],
			"isEnemy": [true, true],
			"entityLimits": {"Foo": 10},
			"entityCounts": {"Foo": 5},
			"entityLimitChangers": {"Foo": {}},
			"researchQueued": new Map(),
			"researchStarted": new Set(),
			"researchedTechs": new Set(),
			"classCounts": {},
			"typeCountsByClass": {},
			"canBarter": false,
			"barterPrices": {
				"buy": { "food": 150 },
				"sell": { "food": 25 }
			},
			"statistics": {
				"resourcesGathered": {
					"food": 100,
					"wood": 0,
					"metal": 0,
					"stone": 0,
					"vegetarianFood": 0
				},
				"percentMapExplored": 10
			},
			"sequences": {
				"unitsTrained": [0, 10],
				"unitsLost": [0, 42],
				"buildingsConstructed": [1, 3],
				"buildingsCaptured": [3, 7],
				"buildingsLost": [3, 10],
				"civCentresBuilt": [4, 10],
				"resourcesGathered": {
					"food": [5, 100],
					"wood": [0, 0],
					"metal": [0, 0],
					"stone": [0, 0],
					"vegetarianFood": [0, 0]
				},
				"treasuresCollected": [1, 20],
				"lootCollected": [0, 2],
				"percentMapExplored": [0, 10],
				"teamPercentMapExplored": [0, 10],
				"percentMapControlled": [0, 10],
				"teamPercentMapControlled": [0, 10],
				"peakPercentOfMapControlled": [0, 10],
				"teamPeakPercentOfMapControlled": [0, 10]
			}
		},
		{
			"name": "Player 2",
			"civ": "mace",
			"color": { "r":1, "g":0, "b":0, "a":1 },
			"controlsAll": true,
			"popCount": 40,
			"popLimit": 30,
			"popMax": 300,
			"panelEntities": [],
			"resourceCounts": { "food": 200 },
			"trainingBlocked": false,
			"state": "active",
			"team": -1,
			"teamsLocked": false,
			"cheatsEnabled": false,
			"disabledTemplates": {},
			"disabledTechnologies": {},
			"hasSharedDropsites": false,
			"hasSharedLos": false,
			"spyCostMultiplier": 1,
			"phase": "village",
			"isAlly": [true, true],
			"isMutualAlly": [false, false],
			"isNeutral": [false, false],
			"isEnemy": [false, false],
			"entityLimits": {"Bar": 20},
			"entityCounts": {"Bar": 0},
			"entityLimitChangers": {"Bar": {}},
			"researchQueued": new Map(),
			"researchStarted": new Set(),
			"researchedTechs": new Set(),
			"classCounts": {},
			"typeCountsByClass": {},
			"canBarter": false,
			"barterPrices": {
				"buy": { "food": 150 },
				"sell": { "food": 25 }
			},
			"statistics": {
				"resourcesGathered": {
					"food": 100,
					"wood": 0,
					"metal": 0,
					"stone": 0,
					"vegetarianFood": 0
				},
				"percentMapExplored": 10
			},
			"sequences": {
				"unitsTrained": [0, 10],
				"unitsLost": [0, 9],
				"buildingsConstructed": [0, 5],
				"buildingsCaptured": [0, 7],
				"buildingsLost": [0, 4],
				"civCentresBuilt": [0, 1],
				"resourcesGathered": {
					"food": [0, 100],
					"wood": [0, 0],
					"metal": [0, 0],
					"stone": [0, 0],
					"vegetarianFood": [0, 0]
				},
				"treasuresCollected": [0, 0],
				"lootCollected": [0, 0],
				"percentMapExplored": [0, 10],
				"teamPercentMapExplored": [0, 10],
				"percentMapControlled": [0, 10],
				"teamPercentMapControlled": [0, 10],
				"peakPercentOfMapControlled": [0, 10],
				"teamPeakPercentOfMapControlled": [0, 10]
			}
		}
	],
	"circularMap": false,
	"timeElapsed": 0,
	"victoryConditions": ["conquest", "wonder"],
	"alliedVictory": false
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
	IsUnhealable: function() { return false; }
});

AddMock(10, IID_Identity, {
	GetClassesList: function() { return ["class1", "class2"]; },
	GetVisibleClassesList: function() { return ["class3", "class4"]; },
	GetRank: function() { return "foo"; },
	GetSelectionGroupName: function() { return "Selection Group Name"; },
	HasClass: function() { return true; },
	IsUndeletable: function() { return false; }
});

AddMock(10, IID_Position, {
	GetTurretParent: function() {return INVALID_ENTITY;},
	GetPosition: function() {
		return {x:1, y:2, z:3};
	},
	IsInWorld: function() {
		return true;
	}
});

AddMock(10, IID_ResourceTrickle, {
	"GetTimer": () => 1250,
	"GetRates": () => ({ "food": 2, "wood": 3, "stone": 5, "metal": 9 })
});

// Note: property order matters when using TS_ASSERT_UNEVAL_EQUALS,
//	because uneval preserves property order. So make sure this object
//	matches the ordering in GuiInterface.
TS_ASSERT_UNEVAL_EQUALS(cmp.GetEntityState(-1, 10), {
	"id": 10,
	"player": INVALID_PLAYER,
	"template": "example",
	"identity": {
		"rank": "foo",
		"classes": ["class1", "class2"],
		"visibleClasses": ["class3", "class4"],
		"selectionGroupName": "Selection Group Name",
		"canDelete": true
	},
	"position": {x:1, y:2, z:3},
	"hitpoints": 50,
	"maxHitpoints": 60,
	"needsRepair": false,
	"needsHeal": true,
	"builder": true,
	"canGarrison": false,
	"visibility": "visible",
	"isBarterMarket":true,
	"resourceTrickle": {
		"interval": 1250,
		"rates": { "food": 2, "wood": 3, "stone": 5, "metal": 9 }
	}
});
