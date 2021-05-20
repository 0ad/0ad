AttackEffects = class AttackEffects
{
	constructor() {}
	Receivers()
	{
		return [{
			"type": "Damage",
			"IID": "IID_Health",
			"method": "TakeDamage"
		},
		{
			"type": "Capture",
			"IID": "IID_Capturable",
			"method": "Capture"
		},
		{
			"type": "ApplyStatus",
			"IID": "IID_StatusEffectsReceiver",
			"method": "ApplyStatus"
		}];
	}
};

Engine.LoadHelperScript("Attack.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Resistance.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Attack.js");

let entityID = 903;

function attackComponentTest(defenderClass, isEnemy, test_function)
{
	let playerEnt1 = 5;

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": () => playerEnt1
	});

	AddMock(playerEnt1, IID_Player, {
		"GetPlayerID": () => 1,
		"IsEnemy": () => isEnemy
	});

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
		"Melee": {
			"Damage": {
				"Hack": 11,
				"Pierce": 5,
				"Crush": 0
			},
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
		"Ranged": {
			"Damage": {
				"Hack": 0,
				"Pierce": 10,
				"Crush": 0
			},
			"MinRange": 10,
			"MaxRange": 80,
			"PrepareTime": 300,
			"RepeatTime": 500,
			"Projectile": {
				"Speed": 10,
				"Spread": 2,
				"Gravity": 1,
				"FriendlyFire": "false"
			},
			"PreferredClasses": {
				"_string": "Archer"
			},
			"RestrictedClasses": {
				"_string": "Elephant"
			},
			"Splash": {
				"Shape": "Circular",
				"Range": 10,
				"FriendlyFire": "false",
				"Damage": {
					"Hack": 0.0,
					"Pierce": 15.0,
					"Crush": 35.0
				},
				"Bonuses": {
					"BonusCav": {
						"Classes": "Cavalry",
						"Multiplier": 3
					}
				}
			}
		},
		"Capture": {
			"Capture": 8,
			"MaxRange": 10,
		},
		"Slaughter": {},
		"StatusEffect": {
			"ApplyStatus": {
				"StatusInternalName": {
					"StatusName": "StatusShownName",
					"ApplierTooltip": "ApplierTooltip",
					"ReceiverTooltip": "ReceiverTooltip",
					"Duration": 5000,
					"Stackability": "Stacks",
					"Modifiers": {
						"SE": {
							"Paths": {
								"_string": "Health/Max"
							},
							"Affects": {
								"_string": "Unit"
							},
							"Add": 10
						}
					}
				}
			},
			"MinRange": "10",
			"MaxRange": "80"
		}
	});

	let defender = ++entityID;

	AddMock(defender, IID_Identity, {
		"GetClassesList": () => [defenderClass],
		"HasClass": className => className == defenderClass,
		"GetCiv": () => "civ"
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

	AddMock(defender, IID_Resistance, {
	});

	test_function(attacker, cmpAttack, defender);
}

// Validate template getter functions
attackComponentTest(undefined, true, (attacker, cmpAttack, defender) => {

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
	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackEffectsData("Capture"), { "Capture": 8 });

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackEffectsData("Ranged"), {
		"Damage": {
			"Hack": 0,
			"Pierce": 10,
			"Crush": 0
		}
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackEffectsData("Ranged", true), {
		"Damage": {
			"Hack": 0.0,
			"Pierce": 15.0,
			"Crush": 35.0
		},
		"Bonuses": {
			"BonusCav": {
				"Classes": "Cavalry",
				"Multiplier": 3
			}
		}
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetAttackEffectsData("StatusEffect"), {
		"ApplyStatus": {
			"StatusInternalName": {
				"Duration": 5000,
				"Interval": 0,
				"Stackability": "Stacks",
				"Modifiers": {
					"SE": {
						"Paths": {
							"_string": "Health/Max"
						},
						"Affects": {
							"_string": "Unit"
						},
						"Add": 10
					}
				}
			}
		}
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Ranged"), {
		"prepare": 300,
		"repeat": 500
	});


	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetRepeatTime("Ranged"), 500);

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetTimers("Capture"), {
		"prepare": 0,
		"repeat": 1000
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetRepeatTime("Capture"), 1000);

	TS_ASSERT_UNEVAL_EQUALS(cmpAttack.GetSplashData("Ranged"), {
		"attackData": {
			"Damage": {
				"Hack": 0,
				"Pierce": 15,
				"Crush": 35,
			},
			"Bonuses": {
				"BonusCav": {
					"Classes": "Cavalry",
					"Multiplier": 3
				}
			}
		},
		"friendlyFire": false,
		"radius": 10,
		"shape": "Circular"
	});
});

for (let className of ["Infantry", "Cavalry"])
	attackComponentTest(className, true, (attacker, cmpAttack, defender) => {

		TS_ASSERT_EQUALS(cmpAttack.GetAttackEffectsData("Melee").Bonuses.BonusCav.Multiplier, 2);

		TS_ASSERT_EQUALS(cmpAttack.GetAttackEffectsData("Capture").Bonuses || null, null);

		let getAttackBonus = (s, t, e, splash) => AttackHelper.GetAttackBonus(s, e, t, cmpAttack.GetAttackEffectsData(t, splash).Bonuses || null);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus(attacker, "Melee", defender), className == "Cavalry" ? 2 : 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus(attacker, "Ranged", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus(attacker, "Ranged", defender, true), className == "Cavalry" ? 3 : 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus(attacker, "Capture", defender), 1);
		TS_ASSERT_UNEVAL_EQUALS(getAttackBonus(attacker, "Slaughter", defender), 1);
	});

// CanAttack rejects elephant attack due to RestrictedClasses
attackComponentTest("Elephant", true, (attacker, cmpAttack, defender) => {
	TS_ASSERT_EQUALS(cmpAttack.CanAttack(defender), false);
});

function testGetBestAttackAgainst(defenderClass, bestAttack, bestAllyAttack, isBuilding = false)
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

		for (let ac of allowCapturing)
			TS_ASSERT_EQUALS(cmpAttack.GetBestAttackAgainst(defender, ac), bestAllyAttack);
	});
}

testGetBestAttackAgainst("FemaleCitizen", "Melee", undefined);
testGetBestAttackAgainst("Archer", "Ranged", undefined);
testGetBestAttackAgainst("Domestic", "Slaughter", "Slaughter");
testGetBestAttackAgainst("Structure", "Capture", "Capture", true);
testGetBestAttackAgainst("Structure", "Ranged", undefined, false);


function testAttackPreference()
{
	const attacker = 5;

	let cmpAttack = ConstructComponent(attacker, "Attack", {
		"Melee": {
			"Damage": {
				"Crush": 0
			},
			"MinRange": 3,
			"MaxRange": 5,
			"PreferredClasses": {
				"_string": "FemaleCitizen Unit+!Ship"
			},
			"RestrictedClasses": {
				"_string": "Elephant Archer"
			},
		}
	});

	AddMock(attacker+1, IID_Identity, {
		"GetClassesList": () => ["FemaleCitizen", "Unit"]
	});

	AddMock(attacker+2, IID_Identity, {
		"GetClassesList": () => ["Unit"]
	});

	AddMock(attacker+3, IID_Identity, {
		"GetClassesList": () => ["Unit", "Ship"]
	});

	AddMock(attacker+4, IID_Identity, {
		"GetClassesList": () => ["SomethingElse"]
	});

	TS_ASSERT_EQUALS(cmpAttack.GetPreference(attacker+1), 0);
	TS_ASSERT_EQUALS(cmpAttack.GetPreference(attacker+2), 1);
	TS_ASSERT_EQUALS(cmpAttack.GetPreference(attacker+3), undefined);
	TS_ASSERT_EQUALS(cmpAttack.GetPreference(attacker+4), undefined);
}
testAttackPreference();
