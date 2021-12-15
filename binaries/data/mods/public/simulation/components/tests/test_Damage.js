AttackEffects = class AttackEffects
{
	constructor() {}
	Receivers()
	{
		return [{
			"type": "Damage",
			"IID": "IID_Health",
			"method": "TakeDamage"
		}];
	}
};

Engine.LoadHelperScript("Attack.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Position.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/DelayedDamage.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Loot.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Resistance.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Attack.js");
Engine.LoadComponentScript("DelayedDamage.js");
Engine.LoadComponentScript("Timer.js");

function Test_Generic()
{
	ResetState();

	let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");
	cmpTimer.OnUpdate({ "turnLength": 1 });
	let attacker = 11;
	let atkPlayerEntity = 1;
	let attackerOwner = 6;
	let cmpAttack = ConstructComponent(attacker, "Attack",
		{
			"Ranged": {
				"Damage": {
					"Crush": 5,
				},
				"MaxRange": 50,
				"MinRange": 0,
				"EffectDelay": 0,
				"Projectile": {
					"Speed": 75.0,
					"Spread": 0.5,
					"Gravity": 9.81,
					"FriendlyFire": "false",
					"LaunchPoint": { "@y": 3 }
				}
			}
		});
	let damage = 5;
	let target = 21;
	let targetOwner = 7;
	let targetPos = new Vector3D(3, 0, 3);

	let type = "Melee";
	let damageTaken = false;

	cmpAttack.GetAttackStrengths = attackType => ({ "Hack": 0, "Pierce": 0, "Crush": damage });

	let data = {
		"type": "Melee",
		"attackData": {
			"Damage": { "Hack": 0, "Pierce": 0, "Crush": damage },
		},
		"target": target,
		"attacker": attacker,
		"attackerOwner": attackerOwner,
		"position": targetPos,
		"projectileId": 9,
		"direction": new Vector3D(1, 0, 0)
	};

	AddMock(atkPlayerEntity, IID_Player, {
		"GetEnemies": () => [targetOwner]
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => atkPlayerEntity,
		"GetAllPlayers": () => [0, 1, 2, 3, 4]
	});

	AddMock(SYSTEM_ENTITY, IID_ProjectileManager, {
		"RemoveProjectile": () => {},
		"LaunchProjectileAtPoint": (ent, pos, speed, gravity) => {},
	});

	AddMock(target, IID_Position, {
		"GetPosition": () => targetPos,
		"GetPreviousPosition": () => targetPos,
		"GetPosition2D": () => Vector2D.From(targetPos),
		"GetHeightAt": () => 0,
		"IsInWorld": () => true,
	});

	AddMock(target, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			damageTaken = true;
			return { "healthChange": -amount };
		},
	});

	AddMock(SYSTEM_ENTITY, IID_DelayedDamage, {
		"Hit": () => {
			damageTaken = true;
		},
	});

	Engine.PostMessage = function(ent, iid, message)
	{
		TS_ASSERT_UNEVAL_EQUALS({
			"type": type,
			"target": target,
			"attacker": attacker,
			"attackerOwner": attackerOwner,
			"damage": damage,
			"capture": 0,
			"statusEffects": [],
			"fromStatusEffect": false
		}, message);
	};

	AddMock(target, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	AddMock(attacker, IID_Ownership, {
		"GetOwner": () => attackerOwner,
	});

	AddMock(attacker, IID_Position, {
		"GetPosition": () => new Vector3D(2, 0, 3),
		"GetRotation": () => new Vector3D(1, 2, 3),
		"IsInWorld": () => true,
	});

	function TestDamage()
	{
		cmpTimer.OnUpdate({ "turnLength": 1 });
		TS_ASSERT(damageTaken);
		damageTaken = false;
	}

	AttackHelper.HandleAttackEffects(target, data);
	TestDamage();

	data.type = "Ranged";
	type = data.type;
	AttackHelper.HandleAttackEffects(target, data);
	TestDamage();

	// Check for damage still being dealt if the attacker dies
	cmpAttack.PerformAttack("Ranged", target);
	Engine.DestroyEntity(attacker);
	TestDamage();

	atkPlayerEntity = 1;
	AddMock(atkPlayerEntity, IID_Player, {
		"GetEnemies": () => [2, 3]
	});
	TS_ASSERT_UNEVAL_EQUALS(AttackHelper.GetPlayersToDamage(atkPlayerEntity, true), [0, 1, 2, 3, 4]);
	TS_ASSERT_UNEVAL_EQUALS(AttackHelper.GetPlayersToDamage(atkPlayerEntity, false), [2, 3]);
}

Test_Generic();

function TestLinearSplashDamage()
{
	ResetState();
	Engine.PostMessage = (ent, iid, message) => {};

	const attacker = 50;
	const attackerOwner = 1;

	const origin = new Vector2D(0, 0);

	let data = {
		"type": "Ranged",
		"attackData": { "Damage": { "Hack": 100, "Pierce": 0, "Crush": 0 } },
		"attacker": attacker,
		"attackerOwner": attackerOwner,
		"origin": origin,
		"radius": 10,
		"shape": "Linear",
		"direction": new Vector3D(1, 747, 0),
		"friendlyFire": false,
	};

	let fallOff = function(x, y)
	{
		return (1 - x * x / (data.radius * data.radius)) * (1 - 25 * y * y / (data.radius * data.radius));
	};

	let hitEnts = new Set();

	AddMock(attackerOwner, IID_Player, {
		"GetEnemies": () => [2]
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => attackerOwner,
		"GetAllPlayers": () => [0, 1, 2]
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [60, 61, 62],
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent) => ({
			"60": Math.sqrt(9.25),
			"61": 0,
			"62": Math.sqrt(29)
		}[ent])
	});

	AddMock(60, IID_Position, {
		"GetPosition2D": () => new Vector2D(3, -0.5),
	});

	AddMock(61, IID_Position, {
		"GetPosition2D": () => new Vector2D(0, 0),
	});

	AddMock(62, IID_Position, {
		"GetPosition2D": () => new Vector2D(5, 2),
	});

	AddMock(60, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(amount, 100 * fallOff(3, -0.5));
			return { "healthChange": -amount };
		}
	});

	AddMock(61, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(61);
			TS_ASSERT_EQUALS(amount, 100 * fallOff(0, 0));
			return { "healthChange": -amount };
		}
	});

	AddMock(62, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(62);
			// Minor numerical precision issues make this necessary
			TS_ASSERT(amount < 0.00001);
			return { "healthChange": -amount };
		}
	});

	AttackHelper.CauseDamageOverArea(data);
	TS_ASSERT(hitEnts.has(60));
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT(hitEnts.has(62));
	hitEnts.clear();

	data.direction = new Vector3D(0.6, 747, 0.8);

	AddMock(60, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(amount, 100 * fallOff(1, 2));
			return { "healthChange": -amount };
		}
	});

	AttackHelper.CauseDamageOverArea(data);
	TS_ASSERT(hitEnts.has(60));
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT(hitEnts.has(62));
	hitEnts.clear();
}

TestLinearSplashDamage();

function TestCircularSplashDamage()
{
	ResetState();
	Engine.PostMessage = (ent, iid, message) => {};

	const radius = 10;
	let attackerOwner = 1;

	let fallOff = function(r)
	{
		return 1 - r * r / (radius * radius);
	};

	AddMock(attackerOwner, IID_Player, {
		"GetEnemies": () => [2]
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => attackerOwner,
		"GetAllPlayers": () => [0, 1, 2]
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [60, 61, 62, 64, 65],
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent, x, z) => ({
			"60": 0,
			"61": 5,
			"62": 1,
			"63": Math.sqrt(85),
			"64": 10,
			"65": 2
		}[ent])
	});

	AddMock(60, IID_Position, {
		"GetPosition2D": () => new Vector2D(3, 4),
	});

	AddMock(61, IID_Position, {
		"GetPosition2D": () => new Vector2D(0, 0),
	});

	AddMock(62, IID_Position, {
		"GetPosition2D": () => new Vector2D(3.6, 3.2),
	});

	AddMock(63, IID_Position, {
		"GetPosition2D": () => new Vector2D(10, -10),
	});

	// Target on the frontier of the shape (see distance above).
	AddMock(64, IID_Position, {
		"GetPosition2D": () => new Vector2D(9, -4),
	});

	// Big target far away (see distance above).
	AddMock(65, IID_Position, {
		"GetPosition2D": () => new Vector2D(23, 4),
	});

	AddMock(60, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(amount, 100 * fallOff(0));
			return { "healthChange": -amount };
		}
	});

	AddMock(61, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(amount, 100 * fallOff(5));
			return { "healthChange": -amount };
		}
	});

	AddMock(62, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(amount, 100 * fallOff(1));
			return { "healthChange": -amount };
		}
	});

	AddMock(63, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT(false);
		}
	});

	let cmphealth64 = AddMock(64, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(amount, 0);
			return { "healthChange": -amount };
		}
	});
	let spy64 = new Spy(cmphealth64, "TakeDamage");

	let cmpHealth65 = AddMock(65, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(amount, 100 * fallOff(2));
			return { "healthChange": -amount };
		}
	});
	let spy65 = new Spy(cmpHealth65, "TakeDamage");

	AttackHelper.CauseDamageOverArea({
		"type": "Ranged",
		"attackData": { "Damage": { "Hack": 100, "Pierce": 0, "Crush": 0 } },
		"attacker": 50,
		"attackerOwner": attackerOwner,
		"origin": new Vector2D(3, 4),
		"radius": radius,
		"shape": "Circular",
		"friendlyFire": false,
	});

	TS_ASSERT_EQUALS(spy64._called, 1);
	TS_ASSERT_EQUALS(spy65._called, 1);
}

TestCircularSplashDamage();

function Test_MissileHit()
{
	ResetState();
	Engine.PostMessage = (ent, iid, message) => {};

	let cmpDelayedDamage = ConstructComponent(SYSTEM_ENTITY, "DelayedDamage");

	let target = 60;
	let targetOwner = 1;
	let targetPos = new Vector3D(3, 10, 0);
	let hitEnts = new Set();

	AddMock(SYSTEM_ENTITY, IID_Timer, {
		"GetLatestTurnLength": () => 500
	});

	const radius = 10;

	let data = {
		"type": "Ranged",
		"attackData": { "Damage": { "Hack": 0, "Pierce": 100, "Crush": 0 } },
		"target": 60,
		"attacker": 70,
		"attackerOwner": 1,
		"position": targetPos,
		"direction": new Vector3D(1, 0, 0),
		"projectileId": 9,
		"friendlyFire": "false",
	};

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => id == 1 ? 10 : 11,
		"GetAllPlayers": () => [0, 1]
	});

	AddMock(SYSTEM_ENTITY, IID_ProjectileManager, {
		"RemoveProjectile": () => {},
		"LaunchProjectileAtPoint": (ent, pos, speed, gravity) => {},
	});

	AddMock(60, IID_Position, {
		"GetPosition": () => targetPos,
		"GetPreviousPosition": () => targetPos,
		"GetPosition2D": () => Vector2D.From(targetPos),
		"IsInWorld": () => true,
	});

	AddMock(60, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(amount, 100);
			return { "healthChange": -amount };
		}
	});

	AddMock(60, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	AddMock(70, IID_Ownership, {
		"GetOwner": () => 1,
	});

	AddMock(70, IID_Position, {
		"GetPosition": () => new Vector3D(0, 0, 0),
		"GetRotation": () => new Vector3D(0, 0, 0),
		"IsInWorld": () => true,
	});

	AddMock(10, IID_Player, {
		"GetEnemies": () => [2]
	});

	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(60));
	hitEnts.clear();

	// Target is a mirage: hit the parent.
	AddMock(60, IID_Mirage, {
		"GetParent": () => 61
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent) => 0
	});

	AddMock(61, IID_Position, {
		"GetPosition": () => targetPos,
		"GetPreviousPosition": () => targetPos,
		"GetPosition2D": () => Vector2D.from3D(targetPos),
		"IsInWorld": () => true
	});

	AddMock(61, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(61);
			TS_ASSERT_EQUALS(amount, 100);
			return { "healthChange": -amount };
		}
	});

	AddMock(61, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 })
	});

	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	hitEnts.clear();

	// Make sure we don't corrupt other tests.
	DeleteMock(60, IID_Mirage);
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(60));
	hitEnts.clear();

	// The main target is not hit but another one is hit.
	AddMock(60, IID_Position, {
		"GetPosition": () => new Vector3D(900, 10, 0),
		"GetPreviousPosition": () => new Vector3D(900, 10, 0),
		"GetPosition2D": () => new Vector2D(900, 0),
		"IsInWorld": () => true
	});

	AddMock(60, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			TS_ASSERT_EQUALS(false);
			return { "healthChange": -amount };
		}
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61]
	});

	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	hitEnts.clear();

	// Add a splash damage.
	data.splash = {};
	data.splash.friendlyFire = false;
	data.splash.radius = 10;
	data.splash.shape = "Circular";
	data.splash.attackData = { "Damage": { "Hack": 0, "Pierce": 0, "Crush": 200 } };

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61, 62]
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent) => ({
			"61": 0,
			"62": 5
		}[ent])
	});

	let dealtDamage = 0;
	AddMock(61, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(61);
			dealtDamage += amount;
			return { "healthChange": -amount };
		}
	});

	AddMock(62, IID_Position, {
		"GetPosition": () => new Vector3D(8, 10, 0),
		"GetPreviousPosition": () => new Vector3D(8, 10, 0),
		"GetPosition2D": () => new Vector2D(8, 0),
		"IsInWorld": () => true,
	});

	AddMock(62, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(62);
			TS_ASSERT_EQUALS(amount, 200 * 0.75);
			return { "healthChange": -amount };
		}
	});

	AddMock(62, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 200);
	dealtDamage = 0;
	TS_ASSERT(hitEnts.has(62));
	hitEnts.clear();

	// Add some hard counters bonus.

	Engine.DestroyEntity(62);
	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61]
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent) => 0
	});

	let bonus = { "BonusCav": { "Classes": "Cavalry", "Multiplier": 400 } };
	let splashBonus = { "BonusCav": { "Classes": "Cavalry", "Multiplier": 10000 } };

	AddMock(61, IID_Identity, {
		"GetClassesList": () => ["Cavalry"],
		"GetCiv": () => "civ"
	});

	data.attackData.Bonuses = bonus;
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 400 * 100 + 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.splash.attackData.Bonuses = splashBonus;
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 400 * 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.attackData.Bonuses = undefined;
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.attackData.Bonuses = null;
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.attackData.Bonuses = {};
	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	// Test splash damage with friendly fire.
	data.splash = {};
	data.splash.friendlyFire = true;
	data.splash.radius = 10;
	data.splash.shape = "Circular";
	data.splash.attackData = { "Damage": { "Pierce": 0, "Crush": 200 } };

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61, 62]
	});

	AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
		"DistanceToPoint": (ent) => ({
			"61": 0,
			"62": 5
		}[ent])
	});

	dealtDamage = 0;
	AddMock(61, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(61);
			dealtDamage += amount;
			return { "healthChange": -amount };
		}
	});

	AddMock(62, IID_Position, {
		"GetPosition": () => new Vector3D(8, 10, 0),
		"GetPreviousPosition": () => new Vector3D(8, 10, 0),
		"GetPosition2D": () => new Vector2D(8, 0),
		"IsInWorld": () => true,
	});

	AddMock(62, IID_Health, {
		"TakeDamage": (amount, __, ___) => {
			hitEnts.add(62);
			TS_ASSERT_EQUALS(amount, 200 * 0.75);
			return { "healtChange": -amount };
		}
	});

	AddMock(62, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	cmpDelayedDamage.Hit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 200);
	dealtDamage = 0;
	TS_ASSERT(hitEnts.has(62));
	hitEnts.clear();
}

Test_MissileHit();
