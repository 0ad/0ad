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
	"projectileId": 9
};

AddMock(atkPlayerEntity, IID_Player, {
	GetEnemies: () => [targetOwner],
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	GetPlayerByID: (id) => atkPlayerEntity,
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	ExecuteQueryAroundPos: () => [target],
	GetElevationAdaptedRange: (pos, rot, max, bonus, a) => max,
});

AddMock(SYSTEM_ENTITY, IID_ProjectileManager, {
	RemoveProjectile: () => {},
	LaunchProjectileAtPoint: (ent, pos, speed, gravity) => {},
});

AddMock(target, IID_Position, {
	GetPosition: () => targetPos,
	GetPreviousPosition: () => targetPos,
	GetPosition2D: () => new Vector2D(3, 3),
	IsInWorld: () => true,
});

AddMock(target, IID_Health, {});

AddMock(target, IID_DamageReceiver, {
	TakeDamage: (hack, pierce, crush) => { damageTaken = true; return { "killed": false, "change": -crush }; },
});

Engine.PostMessage = function(ent, iid, message)
{
	TS_ASSERT_UNEVAL_EQUALS({ "attacker": attacker, "target": target, "type": type, "damage": damage, "attackerOwner": attackerOwner }, message);
};

AddMock(target, IID_Footprint, {
	GetShape: () => ({ "type": "circle", "radius": 20 }),
});

AddMock(attacker, IID_Ownership, {
	GetOwner: () => attackerOwner,
});

AddMock(attacker, IID_Position, {
	GetPosition: () => new Vector3D(2, 0, 3),
	GetRotation: () => new Vector3D(1, 2, 3),
	IsInWorld: () => true,
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
