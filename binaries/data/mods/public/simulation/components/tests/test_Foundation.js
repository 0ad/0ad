Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Cost.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/RallyPoint.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/TerritoryDecay.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("Foundation.js");

let player = 1;
let playerEnt = 3;
let foundationEnt = 20;
let previewEnt = 21;
let newEnt = 22;

function testFoundation(...mocks)
{
	ResetState();

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

	let finalTemplate = "structures/athen_civil_centre.xml";
	let foundationHP = 1;
	let maxHP = 100;
	let rot = new Vector3D(1, 2, 3);
	let pos = new Vector2D(4, 5);

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
		"GetUnitCollisions": () => [],
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
		"JumpTo": (x, y) => {
			TS_ASSERT_EQUALS(x, pos.x);
			TS_ASSERT_EQUALS(y, pos.y);
		},
		"SetYRotation": r => { TS_ASSERT_EQUALS(r, rot.y); },
		"SetXZRotation": (rx, rz) => {
			TS_ASSERT_EQUALS(rx, rot.x);
			TS_ASSERT_EQUALS(rz, rot.z);
		},
	});

	for (let mock of mocks)
		AddMock(...mock);

	// INITIALISE
	Engine.AddEntity = function(template) {
		TS_ASSERT_EQUALS(template, "construction|" + finalTemplate);
		return previewEnt;
	};
	let cmpFoundation = ConstructComponent(foundationEnt, "Foundation", {});
	cmpFoundation.InitialiseConstruction(player, finalTemplate);

	TS_ASSERT_EQUALS(cmpFoundation.owner, player);
	TS_ASSERT_EQUALS(cmpFoundation.finalTemplateName, finalTemplate);
	TS_ASSERT_EQUALS(cmpFoundation.maxProgress, 0);
	TS_ASSERT_EQUALS(cmpFoundation.initialised, true);

	// BUILDER COUNT
	let twoBuilderMultiplier = Math.pow(2, cmpFoundation.buildTimePenalty) / 2;
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 0);
	cmpFoundation.AddBuilder(10);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);
	cmpFoundation.AddBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 2);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, twoBuilderMultiplier);
	cmpFoundation.AddBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 2);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, twoBuilderMultiplier);
	cmpFoundation.RemoveBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);
	cmpFoundation.RemoveBuilder(11);
	TS_ASSERT_EQUALS(cmpFoundation.GetNumBuilders(), 1);
	TS_ASSERT_EQUALS(cmpFoundation.buildMultiplier, 1);

	// COMMIT FOUNDATION
	TS_ASSERT_EQUALS(cmpFoundation.committed, false);
	let work = 5;
	cmpFoundation.Build(10, work);
	TS_ASSERT_EQUALS(cmpFoundation.committed, true);
	TS_ASSERT_EQUALS(foundationHP, 1 + work * cmpFoundation.GetBuildRate() * cmpFoundation.buildMultiplier);
	TS_ASSERT_EQUALS(cmpFoundation.maxProgress, foundationHP / maxHP);

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
	"SelectAnimation": () => {},
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

