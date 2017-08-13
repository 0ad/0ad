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
Engine.LoadComponentScript("interfaces/Sound.js");
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
	let cmpAttack = ConstructComponent(attacker, "Attack", { "Ranged": { "ProjectileSpeed": 500, "Spread": 0.5, "MaxRange": 50, "MinRange": 0 } } );
	let damage = 5;
	let target = 21;
	let targetOwner = 7;
	let targetPos = new Vector3D(3, 0, 3);

	let type = "Melee";
	let damageTaken = false;

	cmpAttack.GetAttackStrengths = (type) => ({ "hack": 0, "pierce": 0, "crush": damage });
	cmpAttack.GetAttackBonus = (type, target) => 1.0;

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
		"GetPlayerByID": (id) => atkPlayerEntity,
		"GetNumPlayers": () => 5
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [target],
		"GetElevationAdaptedRange": (pos, rot, max, bonus, a) => max,
	});

	AddMock(SYSTEM_ENTITY, IID_ProjectileManager, {
		RemoveProjectile: () => {},
		LaunchProjectileAtPoint: (ent, pos, speed, gravity) => {},
	});

	AddMock(target, IID_Position, {
		"GetPosition": () => targetPos,
		"GetPreviousPosition": () => targetPos,
		"GetPosition2D": () => new Vector2D(3, 3),
		"IsInWorld": () => true,
	});

	AddMock(target, IID_Health, {});

	AddMock(target, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => { damageTaken = true; return { "killed": false, "change": -crush }; },
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

	data.friendlyFire = false;
	data.range = 10;
	data.shape = "Circular";
	data.isSplash = true;
	cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Damage, "MissileHit", 1000, data);
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
	}

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
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(2.2, -0.4));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(61, IID_DamageReceiver, {
		TakeDamage: (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(0, 0));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(62, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 0);
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	cmpDamage.CauseSplashDamage(data);

	data.direction = new Vector3D(0.6, 747, 0.8);

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(1, 2));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	cmpDamage.CauseSplashDamage(data);
};

TestLinearSplashDamage();

function TestCircularSplashDamage()
{
	ResetState();
	Engine.PostMessage = (ent, iid, message) => {};

	const radius = 10;

	let fallOff = function(r)
	{
		return 1 - r * r / (radius * radius);
	}

	let cmpDamage = ConstructComponent(SYSTEM_ENTITY, "Damage");

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"ExecuteQueryAroundPos": () => [60, 61, 62],
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
		"GetPosition2D": () => new Vector2D(5, -3),
	});

	// Target on the frontier of the shape
	AddMock(64, IID_Position, {
		"GetPosition2D": () => new Vector2D(4, -2),
	});

	AddMock(60, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(0));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(61, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(5));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(62, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 100 * fallOff(1));
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(63, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 0);
			return { "killed": false, "change": -(hack + pierce + crush) };
		}
	});

	AddMock(64, IID_DamageReceiver, {
		"TakeDamage": (hack, pierce, crush) => {
			TS_ASSERT_EQUALS(hack + pierce + crush, 0);
			return { "killed": false, "change": -(hack + pierce + crush) };
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
};

TestCircularSplashDamage();
