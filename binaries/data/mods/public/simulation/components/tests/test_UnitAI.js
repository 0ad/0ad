Engine.LoadHelperScript("FSM.js");
Engine.LoadHelperScript("Entity.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/DamageReceiver.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Heal.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Pack.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Formation.js");
Engine.LoadComponentScript("UnitAI.js");

/* Regression test.
 * Tests the FSM behaviour of a unit when walking as part of a formation,
 * then exiting the formation.
 * mode == 0: There is no enemy unit nearby.
 * mode == 1: There is a live enemy unit nearby.
 * mode == 2: There is a dead enemy unit nearby.
 */
function TestFormationExiting(mode)
{
	ResetState();

	var playerEntity = 5;
	var unit = 10;
	var enemy = 20;
	var controller = 30;


	AddMock(SYSTEM_ENTITY, IID_Timer, {
		SetInterval: function() { },
		SetTimeout: function() { },
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		CreateActiveQuery: function(ent, minRange, maxRange, players, iid, flags) {
			return 1;
		},
		EnableActiveQuery: function(id) { },
		ResetActiveQuery: function(id) { if (mode == 0) return []; else return [enemy]; },
		DisableActiveQuery: function(id) { },
		GetEntityFlagMask: function(identifier) { },
	});

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		GetCurrentTemplateName: function(ent) { return "formations/line_closed"},
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		GetPlayerByID: function(id) { return playerEntity; },
		GetNumPlayers: function() { return 2; },
	});

	AddMock(playerEntity, IID_Player, {
		IsAlly: function() { return []; },
		IsEnemy: function() { return []; },
	});


	var unitAI = ConstructComponent(unit, "UnitAI", { "FormationController": "false", "DefaultStance": "aggressive" });

	AddMock(unit, IID_Identity, {
		GetClassesList: function() { return []; },
	});

	AddMock(unit, IID_Ownership, {
		GetOwner: function() { return 1; },
	});

	AddMock(unit, IID_Position, {
		GetPosition: function() { return new Vector3D(); },
		GetPosition2D: function() { return new Vector2D(); },
		GetRotation: function() { return { "y": 0 }; },
		IsInWorld: function() { return true; },
	});

	AddMock(unit, IID_UnitMotion, {
		GetWalkSpeed: function() { return 1; },
		MoveToFormationOffset: function(target, x, z) { },
		IsInTargetRange: function(target, min, max) { return true; },
		MoveToTargetRange: function(target, min, max) { },
		StopMoving: function() { },
		GetPassabilityClassName: function() { return "default"; },
		SetPassabilityClassName: function() { },
	});

	AddMock(unit, IID_Vision, {
		GetRange: function() { return 10; },
	});

	AddMock(unit, IID_Attack, {
		GetRange: function() { return { "max": 10, "min": 0}; },
		GetBestAttack: function() { return "melee"; },
		GetBestAttackAgainst: function(t) { return "melee"; },
		GetTimers: function() { return { "prepare": 500, "repeat": 1000 }; },
		CanAttack: function(v) { return true; },
		CompareEntitiesByPreference: function(a, b) { return 0; },
	});

	unitAI.OnCreate();

	unitAI.SetupRangeQuery(1);


	if (mode == 1) 
	{
		AddMock(enemy, IID_Health, {
			GetHitpoints: function() { return 10; },
		});
		AddMock(enemy, IID_UnitAI, {
			IsAnimal: function() { return false; }
		});
	}			
	else if (mode == 2)
		AddMock(enemy, IID_Health, {
			GetHitpoints: function() { return 0; },
		});

	var controllerFormation = ConstructComponent(controller, "Formation", {"FormationName": "Line Closed", "FormationShape": "square", "ShiftRows": "false", "SortingClasses": "", "WidthDepthRatio": 1, "UnitSeparationWidthMultiplier": 1, "UnitSeparationDepthMultiplier": 1, "SpeedMultiplier": 1, "Sloppyness": 0});
	var controllerAI = ConstructComponent(controller, "UnitAI", { "FormationController": "true", "DefaultStance": "aggressive" });

	AddMock(controller, IID_Position, {
		JumpTo: function(x, z) { this.x = x; this.z = z; },
		GetPosition: function() { return new Vector3D(this.x, 0, this.z); },
		GetPosition2D: function() { return new Vector2D(this.x, this.z); },
		GetRotation: function() { return { "y": 0 }; },
		IsInWorld: function() { return true; },
	});

	AddMock(controller, IID_UnitMotion, {
		SetUnitRadius: function(r) { },
		SetSpeed: function(speed) { },
		MoveToPointRange: function(x, z, minRange, maxRange) { },
		GetPassabilityClassName: function() { return "default"; },
		SetPassabilityClassName: function() { },
	});

	controllerAI.OnCreate();


	TS_ASSERT_EQUALS(controllerAI.fsmStateName, "FORMATIONCONTROLLER.IDLE");
	TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.IDLE");

	controllerFormation.SetMembers([unit]);
	controllerAI.Walk(100, 100, false);
	controllerAI.OnMotionChanged({ "starting": true });

	TS_ASSERT_EQUALS(controllerAI.fsmStateName, "FORMATIONCONTROLLER.WALKING");
	TS_ASSERT_EQUALS(unitAI.fsmStateName, "FORMATIONMEMBER.WALKING");

	controllerFormation.Disband();

	if (mode == 0)
		TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.IDLE");
	else if (mode == 1)
		TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.COMBAT.ATTACKING");
	else if (mode == 2)
		TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.IDLE");
	else
		TS_FAIL("invalid mode");
}

function TestMoveIntoFormationWhileAttacking()
{
	ResetState();

	var playerEntity = 5;
	var controller = 10;
	var enemy = 20;
	var unit = 30;
	var units = [];
	var unitCount = 8;
	var unitAIs = [];

	AddMock(SYSTEM_ENTITY, IID_Timer, {
		SetInterval: function() { },
		SetTimeout: function() { },
	});


	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		CreateActiveQuery: function(ent, minRange, maxRange, players, iid, flags) {
			return 1;
		},
		EnableActiveQuery: function(id) { },
		ResetActiveQuery: function(id) { return [enemy]; },
		DisableActiveQuery: function(id) { },
		GetEntityFlagMask: function(identifier) { },
	});;

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		GetCurrentTemplateName: function(ent) { return "formations/line_closed"},
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		GetPlayerByID: function(id) { return playerEntity; },
		GetNumPlayers: function() { return 2; },
	});

	AddMock(playerEntity, IID_Player, {
		IsAlly: function() { return []; },
		IsEnemy: function() { return []; },
	});

	// create units
	for (var i = 0; i < unitCount; i++) {
	
		units.push(unit + i);
		
		var unitAI = ConstructComponent(unit + i, "UnitAI", { "FormationController": "false", "DefaultStance": "aggressive" });
	
		AddMock(unit + i, IID_Identity, {
			GetClassesList: function() { return []; },
		});
	
		AddMock(unit + i, IID_Ownership, {
			GetOwner: function() { return 1; },
		});
	
		AddMock(unit + i, IID_Position, {
			GetPosition: function() { return new Vector3D(); },
			GetPosition2D: function() { return new Vector2D(); },
			GetRotation: function() { return { "y": 0 }; },
			IsInWorld: function() { return true; },
		});
	
		AddMock(unit + i, IID_UnitMotion, {
			GetWalkSpeed: function() { return 1; },
			MoveToFormationOffset: function(target, x, z) { },
			IsInTargetRange: function(target, min, max) { return true; },
			MoveToTargetRange: function(target, min, max) { },
			StopMoving: function() { },
			GetPassabilityClassName: function() { return "default"; },
			SetPassabilityClassName: function() { },
		});
	
		AddMock(unit + i, IID_Vision, {
			GetRange: function() { return 10; },
		});
	
		AddMock(unit + i, IID_Attack, {
			GetRange: function() { return {"max":10, "min": 0}; },
			GetBestAttack: function() { return "melee"; },
			GetBestAttackAgainst: function(t) { return "melee"; },
			GetTimers: function() { return { "prepare": 500, "repeat": 1000 }; },
			CanAttack: function(v) { return true; },
			CompareEntitiesByPreference: function(a, b) { return 0; },
		});
		
		unitAI.OnCreate();
		
		unitAI.SetupRangeQuery(1);
		
		unitAIs.push(unitAI);
	}
	
	// create enemy
	AddMock(enemy, IID_Health, {
		GetHitpoints: function() { return 40; },
	});

	var controllerFormation = ConstructComponent(controller, "Formation", {"FormationName": "Line Closed", "FormationShape": "square", "ShiftRows": "false", "SortingClasses": "", "WidthDepthRatio": 1, "UnitSeparationWidthMultiplier": 1, "UnitSeparationDepthMultiplier": 1, "SpeedMultiplier": 1, "Sloppyness": 0});
	var controllerAI = ConstructComponent(controller, "UnitAI", { "FormationController": "true", "DefaultStance": "aggressive" });

	AddMock(controller, IID_Position, {
		JumpTo: function(x, z) { this.x = x; this.z = z; },
		GetPosition: function() { return new Vector3D(this.x, 0, this.z); },
		GetPosition2D: function() { return new Vector2D(this.x, this.z); },
		GetRotation: function() { return { "y": 0 }; },
		IsInWorld: function() { return true; },
	});

	AddMock(controller, IID_UnitMotion, {
		SetUnitRadius: function(r) { },
		SetSpeed: function(speed) { },
		MoveToPointRange: function(x, z, minRange, maxRange) { },
		IsInTargetRange: function(target, min, max) { return true; },
		GetPassabilityClassName: function() { return "default"; },
		SetPassabilityClassName: function() { },
	});

	AddMock(controller, IID_Attack, {
		GetRange: function() { return {"max":10, "min": 0}; },
		CanAttackAsFormation: function() { return false },
	});

	controllerAI.OnCreate();

	controllerFormation.SetMembers(units);
	
	controllerAI.Attack(enemy, []);
	
	for each (var ent in unitAIs) {
		TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.COMBAT.ATTACKING");
	}
	
	controllerAI.MoveIntoFormation({"name": "Circle"});
	
	// let all units be in position
	for each (var ent in unitAIs) {
		controllerFormation.SetInPosition(ent);
	}

	for each (var ent in unitAIs) {
		TS_ASSERT_EQUALS(unitAI.fsmStateName, "INDIVIDUAL.COMBAT.ATTACKING");
	}
	
	controllerFormation.Disband();
}

TestFormationExiting(0);
TestFormationExiting(1);
TestFormationExiting(2);

TestMoveIntoFormationWhileAttacking();
