Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Transform.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/AutoBuildable.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/Cost.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Guard.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Population.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/Repairable.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/StatusEffectsReceiver.js");
Engine.LoadComponentScript("interfaces/TerritoryDecay.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("AutoBuildable.js");
Engine.LoadComponentScript("Foundation.js");
Engine.LoadComponentScript("Timer.js");

let player = 1;
let playerEnt = 3;
let foundationEnt = 20;
let previewEnt = 21;
let newEnt = 22;
let finalTemplate = "structures/athen/civil_centre.xml";

function testFoundation(...mocks)
{
	ResetState();

	let foundationHP = 1;
	let maxHP = 100;
	let rot = new Vector3D(1, 2, 3);
	let pos = new Vector2D(4, 5);
	let cmpFoundation;

	AddMock(SYSTEM_ENTITY, IID_Trigger, {
		"CallEvent": () => {},
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": () => playerEnt,
	});

	AddMock(SYSTEM_ENTITY, IID_TerritoryManager, {
		"GetOwner": (x, y) => {
			TS_ASSERT_EQUALS(x, pos.x);
			TS_ASSERT_EQUALS(y, pos.y);
			return player;
		},
	});

	Engine.RegisterGlobal("PlaySound", (name, source) => {
		TS_ASSERT_EQUALS(name, "constructed");
		TS_ASSERT_EQUALS(source, newEnt);
	});
	Engine.RegisterGlobal("MT_EntityRenamed", "entityRenamed");

	AddMock(foundationEnt, IID_Cost, {
		"GetBuildTime": () => 50,
		"GetResourceCosts": () => ({ "wood": 100 }),
	});

	AddMock(foundationEnt, IID_Health, {
		"GetHitpoints": () => foundationHP,
		"GetMaxHitpoints": () => maxHP,
		"Increase": hp => {
			foundationHP = Math.min(foundationHP + hp, maxHP);
			cmpFoundation.OnHealthChanged();
		},
	});

	AddMock(foundationEnt, IID_Obstruction, {
		"GetBlockMovementFlag": () => true,
		"GetEntitiesBlockingConstruction": () => [],
		"GetEntitiesDeletedUponConstruction": () => [],
		"SetDisableBlockMovementPathfinding": () => {},
	});

	AddMock(foundationEnt, IID_Ownership, {
		"GetOwner": () => player,
	});

	AddMock(foundationEnt, IID_Position, {
		"GetPosition2D": () => pos,
		"GetRotation": () => rot,
		"SetConstructionProgress": () => {},
		"IsInWorld": () => true,
		"GetHeightOffset": () => {},
		"MoveOutOfWorld": () => {}
	});

	AddMock(previewEnt, IID_Ownership, {
		"SetOwner": owner => { TS_ASSERT_EQUALS(owner, player); },
	});

	AddMock(previewEnt, IID_Position, {
		"JumpTo": (x, y) => {
			TS_ASSERT_EQUALS(x, pos.x);
			TS_ASSERT_EQUALS(y, pos.y);
		},
		"SetConstructionProgress": p => {},
		"SetYRotation": r => { TS_ASSERT_EQUALS(r, rot.y); },
		"SetXZRotation": (rx, rz) => {
			TS_ASSERT_EQUALS(rx, rot.x);
			TS_ASSERT_EQUALS(rz, rot.z);
		},
	});

	AddMock(newEnt, IID_Ownership, {
		"SetOwner": owner => { TS_ASSERT_EQUALS(owner, player); },
	});

	AddMock(newEnt, IID_Position, {
		"GetPosition2D": () => pos,
		"JumpTo": (x, y) => {
			TS_ASSERT_EQUALS(x, pos.x);
			TS_ASSERT_EQUALS(y, pos.y);
		},
		"SetYRotation": r => { TS_ASSERT_EQUALS(r, rot.y); },
		"SetXZRotation": (rx, rz) => {
			TS_ASSERT_EQUALS(rx, rot.x);
			TS_ASSERT_EQUALS(rz, rot.z);
		},
		"SetHeightOffset": () => {}
	});

	for (let mock of mocks)
		AddMock(...mock);

	// INITIALISE
	Engine.AddLocalEntity = function(template) {
		TS_ASSERT_EQUALS(template, "construction|" + finalTemplate);
		return previewEnt;
	};
	cmpFoundation = ConstructComponent(foundationEnt, "Foundation", {
		"BuildTimeModifier": "0.7"
	});
	cmpFoundation.InitialiseConstruction(finalTemplate);

	TS_ASSERT_EQUALS(cmpFoundation.finalTemplateName, finalTemplate);
	TS_ASSERT_EQUALS(cmpFoundation.maxProgress, 0);
	TS_ASSERT_EQUALS(cmpFoundation.initialised, true);

	// BUILDER COUNT, BUILD RATE, TIME REMAINING
	AddMock(10, IID_Builder, { "GetRate": () => 1.0 });
	AddMock(11, IID_Builder, { "GetRate": () => 1.0 });
	let twoBuilderMultiplier = Math.pow(2, cmpFoundation.buildTimeModifier) / 2;
	let threeBuilderMultiplier = Math.pow(3, cmpFoundation.buildTimeModifier) / 3;

	TS_ASSERT_EQUALS(cmpFoundation.CalculateBuildMultiplier(1), 1);
	TS_ASSERT_EQUALS(cmpFoundation.CalculateBuildMultiplier(2), twoBuilderMultiplier);
	TS_ASSERT_EQUALS(cmpFoundation.CalculateBuildMultiplier(3), threeBuilderMultiplier);

	TS_ASSERT_EQUALS(cmpFoundation.GetBuildRate(), 2);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 0);
	TS_ASSERT_EQUALS(cmpFoundation.totalBuilderRate, 0);
	cmpFoundation.AddBuilder(10);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);
	TS_ASSERT_EQUALS(cmpFoundation.totalBuilderRate, 1);
	// Foundation starts with 1 hp, so there's 50 * 99/100 = 49.5 seconds left.
	TS_ASSERT_UNEVAL_EQUALS(cmpFoundation.GetBuildTime(), {
		'timeRemaining': 49.5,
		'timeRemainingNew': 49.5 / (2 * twoBuilderMultiplier)
	});
	cmpFoundation.AddBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 2);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, twoBuilderMultiplier);
	TS_ASSERT_EQUALS(cmpFoundation.totalBuilderRate, 2);
	TS_ASSERT_UNEVAL_EQUALS(cmpFoundation.GetBuildTime(), {
		'timeRemaining': 49.5 / (2 * twoBuilderMultiplier),
		'timeRemainingNew': 49.5 / (3 * threeBuilderMultiplier)
	});
	cmpFoundation.AddBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 2);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, twoBuilderMultiplier);
	cmpFoundation.RemoveBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);
	cmpFoundation.RemoveBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);
	TS_ASSERT_EQUALS(cmpFoundation.totalBuilderRate, 1);

	// COMMIT FOUNDATION
	TS_ASSERT_EQUALS(cmpFoundation.committed, false);
	let work = 5;
	cmpFoundation.Build(10, work);
	TS_ASSERT_EQUALS(cmpFoundation.committed, true);
	TS_ASSERT_EQUALS(foundationHP, 1 + work * cmpFoundation.GetBuildRate() * cmpFoundation.buildMultiplier);
	TS_ASSERT_EQUALS(cmpFoundation.maxProgress, foundationHP / maxHP);
	TS_ASSERT_EQUALS(cmpFoundation.totalBuilderRate, 5);

	// FINISH CONSTRUCTION
	Engine.AddEntity = function(template) {
		TS_ASSERT_EQUALS(template, finalTemplate);
		return newEnt;
	};
	cmpFoundation.Build(10, 1000);
	TS_ASSERT_EQUALS(cmpFoundation.maxProgress, 1);
	TS_ASSERT_EQUALS(foundationHP, maxHP);
}

testFoundation();

testFoundation([foundationEnt, IID_Visual, {
	"SetVariable": (key, num) => {
		TS_ASSERT_EQUALS(key, "numbuilders");
		TS_ASSERT(num == 1 || num == 2);
	},
	"SelectAnimation": (name, once, speed) => name,
	"HasConstructionPreview": () => true,
}]);

testFoundation([newEnt, IID_TerritoryDecay, {
	"HasTerritoryOwnership": () => true,
}]);

testFoundation([playerEnt, IID_StatisticsTracker, {
	"IncreaseConstructedBuildingsCounter": ent => {
		TS_ASSERT_EQUALS(ent, newEnt);
	},
}]);

// Test autobuild feature.
const foundationEnt2 = 42;
let turnLength = 0.2;
let currentFoundationHP = 1;
let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");

AddMock(foundationEnt2, IID_Cost, {
	"GetBuildTime": () => 50,
	"GetResourceCosts": () => ({ "wood": 100 }),
});



const cmpAutoBuildingFoundation = ConstructComponent(foundationEnt2, "Foundation", {});
AddMock(foundationEnt2, IID_Health, {
	"GetHitpoints": () => currentFoundationHP,
	"GetMaxHitpoints": () => 100,
	"Increase": hp => {
		currentFoundationHP = Math.min(currentFoundationHP + hp, 100);
		cmpAutoBuildingFoundation.OnHealthChanged();
	},
});

const cmpBuildableAuto = ConstructComponent(foundationEnt2, "AutoBuildable", {
	"Rate": "1.0"
});

cmpAutoBuildingFoundation.InitialiseConstruction(finalTemplate);

// We start at 3 cause there is no delay on the first run.
cmpTimer.OnUpdate({ "turnLength": turnLength });

for (let i = 0; i < 10; ++i)
{
	if (i == 8)
	{
		cmpBuildableAuto.CancelTimer();
		TS_ASSERT_EQUALS(cmpAutoBuildingFoundation.GetNumBuilders(), 0);
	}

	let currentPercentage = cmpAutoBuildingFoundation.GetBuildPercentage();
	cmpTimer.OnUpdate({ "turnLength": turnLength * 5 });
	let newPercentage = cmpAutoBuildingFoundation.GetBuildPercentage();

	if (i >= 8)
		TS_ASSERT_EQUALS(currentPercentage, newPercentage);
	else
		// Rate * Max Health / Cost.
		TS_ASSERT_EQUALS(currentPercentage + 2, newPercentage);
}
