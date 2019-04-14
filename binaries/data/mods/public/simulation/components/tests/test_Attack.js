Engine.LoadHelperScript("DamageBonus.js");
Engine.LoadHelperScript("DamageTypes.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Attack.js");

let entityID = 903;

function attackComponentTest(defenderClass, isEnemy, test_function)
{
	ResetState();

	{
		let playerEnt1 = 5;

		AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
			"GetPlayerByID": () => playerEnt1
		});

		AddMock(playerEnt1, IID_Player, {
			"GetPlayerID": () => 1,
			"IsEnemy": () => isEnemy
		});
	}

	let attacker = entityID;

	AddMock(attacker, IID_Position, {
		"IsInWorld": () => true,
		"GetHeightOffset": () => 5,
		"GetPosition2D": () => new Vector2D(1, 2)
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
			"Projectile": {
				"Speed": 10,
				"Spread": 2,
				"Gravity": 1
			},
			"PreferredClasses": {
				"_string": "Archer"
			},
			"RestrictedClasses": {
				"_string": "Elephant"
			},
			"Splash" : {
				"Shape": "Circular",
				"Range": 10,
				"FriendlyFire": "false",
				"Hack": 0.0,
				"Pierce": 15.0,
				"Crush": 35.0,
				"Bonuses": {
					"BonusCav": {
						"Classes": "Cavalry",
						"Multiplier": 3
					}
				}
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

	AddMock(defender, IID_Ownership, {
		"GetOwner": () => 1
	});

	AddMock(defender, IID_Position, {
		"IsInWorld": () => true,
		"GetHeightOffset": () => 0
	});

	AddMock(defender, IID_Health, {
		"GetHitpoints": () => 100
	});

	test_function(attacker, cmpAttack, defender);
}

// Validate template getter functions
attackComponentTest(undefined, true ,(attacker, cmpAttack, defender) => {

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(), ["Melee", "Ranged", "Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes([]), ["Melee", "Ranged", "Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Melee", "Ranged", "Capture"]), ["Melee", "Ranged", "Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Melee", "Ranged"]), ["Melee", "Ranged"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Capture"]), ["Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Melee", "!Melee"]), []);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["!Melee"]), ["Ranged", "Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["!Melee", "!Ranged"]), ["Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Capture", "!Ranged"]), ["Capture"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackTypes(["Capture", "Melee", "!Ranged"]), ["Melee", "Capture"]);

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetPreferredClasses("Melee"), ["FemaleCitizen"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetRestrictedClasses("Melee"), ["Elephant", "Archer"]);
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetFullAttackRange(), { "min": 0, "max": 80 });
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackStrengths("Capture"), { "value": 8 });

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackStrengths("Ranged"), {
		"Hack": 0,
		"Pierce": 10,
		"Crush": 0
	});
	
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackStrengths("Ranged.Splash"), {
		"Hack": 0.0,
		"Pierce": 15.0,
		"Crush": 35.0
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Ranged"), {
		"prepare": 300,
		"repeat": 500
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Capture"), {
		"prepare": 0,
		"repeat": 1000
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetSplashDamage("Ranged"), {
		"Hack": 0,
		"Pierce": 15,
		"Crush": 35,
		"friendlyFire": false,
		"shape": "Circular"
	});
});

for (let className of ["Infantry", "Cavalry"])
	attackComponentTest(className, true, (attacker, cmpAttack, defender) => {

		TS_ASSERT_EQUALS(cmpAttack.GetBonusTemplate("Melee").BonusCav.Multiplier, 2);

		TS_ASSERT(cmpAttack.GetBonusTemplate("Capture") === null);

		let getAttackBonus = (t, e) => GetDamageBonus(e, cmpAttack.GetBonusTemplate(t));
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus("Melee", defender), className == "Cavalry" ? 2 : 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus("Ranged", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus("Ranged.Splash", defender), className == "Cavalry" ? 3 : 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus("Capture", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus("Slaughter", defender), 1);
	});

// CanAttack rejects elephant attack due to RestrictedClasses
attackComponentTest("Elephant", true, (attacker, cmpAttack, defender) => {
	TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), false);
});

function testGetBestAttackAgainst(defenderClass, bestAttack, isBuilding = false)
{
	attackComponentTest(defenderClass, true, (attacker, cmpAttack, defender) => {

		if (isBuilding)
			AddMock(defender, IID_Capturable, {
				"CanCapture": playerID => {
					TS_ASSERT_EQUALS(playerID, 1);
					return true;
				}
			});

		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), true);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, []), true);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Ranged"]), true);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["!Melee"]), true);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Capture"]), isBuilding);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Melee", "Capture"]), defenderClass != "Archer");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Ranged", "Capture"]), true);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["!Ranged", "!Melee"]), isBuilding || defenderClass == "Domestic");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Melee", "!Melee"]), false);

		let allowCapturing = [true];
		if (!isBuilding)
			allowCapturing.push(false);

		for (let ac of allowCapturing)
			TS_ASSERT_EQUALS(cmpAttack.GetBestAttackAgainst(defender, ac), bestAttack);
	});

	attackComponentTest(defenderClass, false, (attacker, cmpAttack, defender) => {

		if (isBuilding)
			AddMock(defender, IID_Capturable, {
				"CanCapture": playerID => {
					TS_ASSERT_EQUALS(playerID, 1);
					return true;
				}
			});

		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), isBuilding || defenderClass == "Domestic");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, []), isBuilding || defenderClass == "Domestic");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Ranged"]), false);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["!Melee"]), isBuilding || defenderClass == "Domestic");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Capture"]), isBuilding);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Melee", "Capture"]), isBuilding);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Ranged", "Capture"]), isBuilding);
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["!Ranged", "!Melee"]), isBuilding || defenderClass == "Domestic");
		TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender, ["Melee", "!Melee"]), false);

		let allowCapturing = [true];
		if (!isBuilding)
			allowCapturing.push(false);

		let attack;
		if (defenderClass == "Domestic")
			attack = "Slaughter";
		else if (defenderClass == "Structure")
			attack = "Capture";

		for (let ac of allowCapturing)
			TS_ASSERT_EQUALS(cmpAttack.GetBestAttackAgainst(defender, ac), bestAttack);
	});
}

testGetBestAttackAgainst("FemaleCitizen", "Melee");
testGetBestAttackAgainst("Archer", "Ranged");
testGetBestAttackAgainst("Domestic", "Slaughter");
testGetBestAttackAgainst("Structure", "Capture", true);

function testPredictTimeToTarget(selfPosition, horizSpeed, targetPosition, targetVelocity)
{
	ResetState();
	let cmpAttack = ConstructComponent(1, "Attack", {});
	let timeToTarget = cmpAttack.PredictTimeToTarget(selfPosition, horizSpeed, targetPosition, targetVelocity);
	if (timeToTarget === false)
		return;
	// Position of the target after that time.
	let targetPos = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);
	// Time that the projectile need to reach it.
	let time = targetPos.horizDistanceTo(selfPosition) / horizSpeed;
	TS_ASSERT_EQUALS(timeToTarget.toFixed(1), time.toFixed(1));
}

testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(0, 0, 0), new Vector3D(0, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(0, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(1, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(4, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(16, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-1, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-4, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-16, 0, 0));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(0, 0, 1));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(0, 0, 4));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(0, 0, 16));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(1, 0, 1));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(2, 0, 2));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(8, 0, 8));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-1, 0, 1));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-2, 0, 2));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-8, 0, 8));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(4, 0, 2));
testPredictTimeToTarget(new Vector3D(0, 0, 0), 4, new Vector3D(20, 0, 0), new Vector3D(-4, 0, 2));
