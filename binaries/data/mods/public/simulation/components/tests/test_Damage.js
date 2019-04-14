Engine.LoadHelperScript("DamageBonus.js");
Engine.LoadHelperScript("DamageTypes.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Sound.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/AttackDetection.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Damage.js");
Engine.LoadComponentScript("interfaces/DamageReceiver.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Loot.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("Attack.js");
Engine.LoadComponentScript("Damage.js");
Engine.LoadComponentScript("Timer.js");

function Test_Generic()
{
	ResetState();

	let cmpDamage = ConstructComponent(SYSTEM_ENTITY, "Damage");
	let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");
	cmpTimer.OnUpdate({ turnLength: 1 });
	let attacker = 11;
	let atkPlayerEntity = 1;
	let attackerOwner = 6;
	let cmpAttack = ConstructComponent(attacker, "Attack",
		{
			"Ranged": {
				"MaxRange": 50,
				"MinRange": 0,
				"Delay": 0,
				"Projectile": {
					"Speed": 75.0,
					"Spread": 0.5,
					"Gravity": 9.81,
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

	cmpAttack.GetAttackStrengths = attackType => ({ "hack": 0, "pierce": 0, "crush": damage });

	let data = {
		"attacker": attacker,
		"target": target,
		"type": "Melee",
		"strengths": { "hack": 0, "pierce": 0, "crush": damage },
		"multiplier": 1.0,
		"attackerOwner": attackerOwner,
		"position": targetPos,
		"isSplash": false,
		"projectileId": 9,
		"direction": new Vector3D(1,0,0)
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
		"IsInWorld": () => true,
	});

	AddMock(target, IID_Health, {});

	AddMock(target, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => { damageTaken = true; return { "killed": false, "change": -multiplier * strengths.crush }; },
	});

	Engine.PostMessage = function(ent, iid, message)
	{
		TS_ASSERT_UNEVAL_EQUALS({ "attacker": attacker, "target": target, "type": type, "damage": damage, "attackerOwner": attackerOwner }, message);
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
		cmpTimer.OnUpdate({ turnLength: 1 });
		TS_ASSERT(damageTaken);
		damageTaken = false;
	}

	cmpDamage.CauseDamage(data);
	TestDamage();

	type = data.type = "Ranged";
	cmpDamage.CauseDamage(data);
	TestDamage();

	// Check for damage still being dealt if the attacker dies
	cmpAttack.PerformAttack("Ranged", target);
	Engine.DestroyEntity(attacker);
	TestDamage();

	atkPlayerEntity = 1;
	AddMock(atkPlayerEntity, IID_Player, {
		"GetEnemies": () => [2, 3]
	});
	TS_ASSERT_UNEVAL_EQUALS(cmpDamage.GetPlayersToDamage(atkPlayerEntity, true), [0, 1, 2, 3, 4]);
	TS_ASSERT_UNEVAL_EQUALS(cmpDamage.GetPlayersToDamage(atkPlayerEntity, false), [2, 3]);
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
		"attacker": attacker,
		"origin": origin,
		"radius": 10,
		"shape": "Linear",
		"strengths": { "hack" : 100, "pierce" : 0, "crush": 0 },
		"direction": new Vector3D(1, 747, 0),
		"playersToDamage": [2],
		"type": "Ranged",
		"attackerOwner": attackerOwner
	};

	let fallOff = function(x,y)
	{
		return (1 - x * x / (data.radius * data.radius)) * (1 - 25 * y * y / (data.radius * data.radius));
	};

	let hitEnts = new Set();

	let cmpDamage = ConstructComponent(SYSTEM_ENTITY, "Damage");

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [60, 61, 62],
	});

	AddMock(60, IID_Position, {
		"GetPosition2D": () => new Vector2D(2.2, -0.4),
	});

	AddMock(61, IID_Position, {
		"GetPosition2D": () => new Vector2D(0, 0),
	});

	AddMock(62, IID_Position, {
		"GetPosition2D": () => new Vector2D(5, 2),
	});

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(2.2, -0.4));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(61, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			hitEnts.add(61);
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(0, 0));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(62, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			hitEnts.add(62);
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 0);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	cmpDamage.CauseSplashDamage(data);
	TS_ASSERT(hitEnts.has(60));
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT(hitEnts.has(62));
	hitEnts.clear();

	data.direction = new Vector3D(0.6, 747, 0.8);

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(1, 2));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	cmpDamage.CauseSplashDamage(data);
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

	let fallOff = function(r)
	{
		return 1 - r * r / (radius * radius);
	};

	let cmpDamage = ConstructComponent(SYSTEM_ENTITY, "Damage");

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [60, 61, 62, 64],
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

	// Target on the frontier of the shape
	AddMock(64, IID_Position, {
		"GetPosition2D": () => new Vector2D(9, -4),
	});

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(0));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(61, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(5));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(62, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100 * fallOff(1));
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(63, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT(false);
		}
	});

	AddMock(64, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 0);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	cmpDamage.CauseSplashDamage({
		"attacker": 50,
		"origin": new Vector2D(3, 4),
		"radius": radius,
		"shape": "Circular",
		"strengths": { "hack" : 100, "pierce" : 0, "crush": 0 },
		"playersToDamage": [2],
		"type": "Ranged",
		"attackerOwner": 1
	});
}

TestCircularSplashDamage();

function Test_MissileHit()
{
	ResetState();
	Engine.PostMessage = (ent, iid, message) => {};

	let cmpDamage = ConstructComponent(SYSTEM_ENTITY, "Damage");

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
		"attacker": 70,
		"target": 60,
		"strengths": { "hack": 0, "pierce": 100, "crush": 0 },
		"position": targetPos,
		"direction": new Vector3D(1, 0, 0),
		"projectileId": 9,
		"bonus": undefined,
		"isSplash": false,
		"attackerOwner": 1
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

	AddMock(60, IID_Health, {});

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			hitEnts.add(60);
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
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

	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(60));
	hitEnts.clear();

	// The main target is not hit but another one is hit.

	AddMock(60, IID_Position, {
		"GetPosition": () => new Vector3D(900, 10, 0),
		"GetPreviousPosition": () => new Vector3D(900, 10, 0),
		"GetPosition2D": () => new Vector2D(900, 0),
		"IsInWorld": () => true,
	});

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(false);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61]
	});

	AddMock(61, IID_Position, {
		"GetPosition": () => targetPos,
		"GetPreviousPosition": () => targetPos,
		"GetPosition2D": () => Vector2D.from3D(targetPos),
		"IsInWorld": () => true,
	});

	AddMock(61, IID_Health, {});

	AddMock(61, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 100);
			hitEnts.add(61);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(61, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	hitEnts.clear();

	// Add a splash damage.

	data.friendlyFire = false;
	data.radius = 10;
	data.shape = "Circular";
	data.isSplash = true;
	data.splashStrengths = { "hack": 0, "pierce": 0, "crush": 200 };

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [61, 62]
	});

	let dealtDamage = 0;
	AddMock(61, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			dealtDamage += multiplier * (strengths.hack + strengths.pierce + strengths.crush);
			hitEnts.add(61);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(62, IID_Position, {
		"GetPosition": () => new Vector3D(8, 10, 0),
		"GetPreviousPosition": () => new Vector3D(8, 10, 0),
		"GetPosition2D": () => new Vector2D(8, 0),
		"IsInWorld": () => true,
	});

	AddMock(62, IID_Health, {});

	AddMock(62, IID_DamageReceiver, {
		"TakeDamage": (strengths, multiplier) => {
			TS_ASSERT_EQUALS(multiplier * (strengths.hack + strengths.pierce + strengths.crush), 200 * 0.75);
			hitEnts.add(62);
			return { "killed": false, "change": -multiplier * (strengths.hack + strengths.pierce + strengths.crush) };
		}
	});

	AddMock(62, IID_Footprint, {
		"GetShape": () => ({ "type": "circle", "radius": 20 }),
	});

	cmpDamage.MissileHit(data, 0);
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

	let bonus= { "BonusCav": { "Classes": "Cavalry", "Multiplier": 400 } };
	let splashBonus = { "BonusCav": { "Classes": "Cavalry", "Multiplier": 10000 } };

	AddMock(61, IID_Identity, {
		"HasClass": cl => cl == "Cavalry"
	});

	data.bonus = bonus;
	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 400 * 100 + 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.splashBonus = splashBonus;
	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 400 * 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.bonus = undefined;
	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.bonus = null;
	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();

	data.bonus = {};
	cmpDamage.MissileHit(data, 0);
	TS_ASSERT(hitEnts.has(61));
	TS_ASSERT_EQUALS(dealtDamage, 100 + 10000 * 200);
	dealtDamage = 0;
	hitEnts.clear();
}

Test_MissileHit();
