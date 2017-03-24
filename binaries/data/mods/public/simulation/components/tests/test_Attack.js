Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("Attack.js");

let entityID = 903;

function attackComponentTest(defenderClass, test_function)
{
	ResetState();

	{
		let playerEnt1 = 5;

		AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
			"GetPlayerByID": () => playerEnt1
		});

		AddMock(playerEnt1, IID_Player, {
			"GetPlayerID": () => 1
		});
	}

	let attacker = entityID;

	AddMock(attacker, IID_Position, {
		"IsInWorld": () => true,
		"GetHeightOffset": () => 5
	});

	AddMock(attacker, IID_Ownership, {
		"GetOwner": () => 1
	});

	let cmpAttack = ConstructComponent(attacker, "Attack", {
		"Melee" : {
			"Hack": 11,
			"Pierce": 5,
			"Crush": 0,
			"MinRange": 3,
			"MaxRange": 5,
			"PreferredClasses": {
				"_string": "FemaleCitizen"
			},
			"RestrictedClasses": {
				"_string": "Elephant Archer"
			},
			"Bonuses":
			{
				"BonusCav": {
					"Classes": "Cavalry",
					"Multiplier": 2
				}
			}
		},
		"Ranged" : {
			"Hack": 0,
			"Pierce": 10,
			"Crush": 0,
			"MinRange": 10,
			"MaxRange": 80,
			"PrepareTime": 300,
			"RepeatTime": 500,
			"PreferredClasses": {
				"_string": "Archer"
			},
			"RestrictedClasses": {
				"_string": "Elephant"
			}
		},
		"Capture" : {
			"Value": 8,
			"MaxRange": 10,
		},
		"Slaughter": {}
	});

	let defender = ++entityID;

	AddMock(defender, IID_Identity, {
		"GetClassesList": () => [defenderClass],
		"HasClass": className => className == defenderClass
	});

	AddMock(defender, IID_Position, {
		"IsInWorld": () => true,
		"GetHeightOffset": () => 0
	});

	test_function(attacker, cmpAttack, defender);
}

// Validate template getter functions
attackComponentTest(undefined, (attacker, cmpAttack, defender) => {

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(), ["Melee", "Ranged", "Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetPreferredClasses("Melee"), ["FemaleCitizen"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetRestrictedClasses("Melee"), ["Elephant", "Archer"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetFullAttackRange(), { "min": 0, "max": 80 });
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackStrengths("Capture"), { "value": 8 });

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackStrengths("Ranged"), {
		"hack": 0,
		"pierce": 10,
		"crush": 0
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Ranged"), {
		"prepare": 300,
		"repeat": 500
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Capture"), {
		"prepare": 0,
		"repeat": 1000
	});
});

for (let className of ["Infantry", "Cavalry"])
	attackComponentTest(className, (attacker, cmpAttack, defender) => {
		TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackBonus("Melee", defender), className == "Cavalry" ? 2 : 1);
		TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackBonus("Ranged", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackBonus("Capture", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackBonus("Slaughter", defender), 1);
	});

// CanAttack rejects elephant attack due to RestrictedClasses
attackComponentTest("Elephant", (attacker, cmpAttack, defender) => {
	TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), false);
});

function testGetBestAttackAgainst(defenderClass, bestAttack, isBuilding = false)
{
	attackComponentTest(defenderClass, (attacker, cmpAttack, defender) => {

		if (isBuilding)
			AddMock(defender, IID_Capturable, {
				"CanCapture": playerID => {
					TS_ASSERT_EQUALS(playerID, 1);
					return true;
				}
			});

		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), true);

		let allowCapturing = [true];
		if (!isBuilding)
			allowCapturing.push(false);

		for (let ac of allowCapturing)
			TS_ASSERT_EQUALS(cmpAttack.GetBestAttackAgainst(defender, ac), bestAttack);
	});
}

testGetBestAttackAgainst("FemaleCitizen", "Melee");
testGetBestAttackAgainst("Archer", "Ranged");
testGetBestAttackAgainst("Domestic", "Slaughter");
testGetBestAttackAgainst("Structure", "Capture", true);
