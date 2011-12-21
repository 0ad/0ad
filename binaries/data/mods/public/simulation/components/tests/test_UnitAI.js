Engine.LoadHelperScript("FSM.js");
Engine.LoadHelperScript("Entity.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/DamageReceiver.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Health.js");
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
		SetTimeout: function() { },
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		CreateActiveQuery: function(ent, minRange, maxRange, players, iid) {
			return 1;
		},
		EnableActiveQuery: function(id) { },
		ResetActiveQuery: function(id) { if (mode == 0) return []; else return [enemy]; },
		DisableActiveQuery: function(id) { },
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
		GetPosition: function() { return { "x": 0, "z": 0 }; },
		IsInWorld: function() { return true; },
	});

	AddMock(unit, IID_UnitMotion, {
		GetWalkSpeed: function() { return 1; },
		MoveToFormationOffset: function(target, x, z) { },
		IsInTargetRange: function(target, min, max) { return true; },
		MoveToTargetRange: function(target, min, max) { },
		StopMoving: function() { },
	});

	AddMock(unit, IID_Vision, {
		GetRange: function() { return 10; },
	});

	AddMock(unit, IID_Attack, {
		GetRange: function() { return 10; },
		GetBestAttack: function() { return "melee"; },
		GetTimers: function() { return { "prepare": 500, "repeat": 1000 }; },
	});

	unitAI.OnCreate();

	unitAI.SetupRangeQuery(1);


	if (mode == 1)
		AddMock(enemy, IID_Health, {
			GetHitpoints: function() { return 10; },
		});
	else if (mode == 2)
		AddMock(enemy, IID_Health, {
			GetHitpoints: function() { return 0; },
		});

	var controllerFormation = ConstructComponent(controller, "Formation");
	var controllerAI = ConstructComponent(controller, "UnitAI", { "FormationController": "true", "DefaultStance": "aggressive" });

	AddMock(controller, IID_Position, {
		JumpTo: function(x, z) { this.x = x; this.z = z; },
		GetPosition: function() { return { "x": this.x, "z": this.z }; },
		IsInWorld: function() { return true; },
	});

	AddMock(controller, IID_UnitMotion, {
		SetUnitRadius: function(r) { },
		SetSpeed: function(speed) { },
		MoveToPointRange: function(x, z, minRange, maxRange) { },
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

TestFormationExiting(0);
TestFormationExiting(1);
TestFormationExiting(2);
