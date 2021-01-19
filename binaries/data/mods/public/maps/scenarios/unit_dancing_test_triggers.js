const ARCHER_TEMPLATE = "units/maur/infantry_archer_b";
const JAV_TEMPLATE = "units/mace/infantry_javelineer_b";

const REG_UNIT_TEMPLATE = "units/athen/infantry_spearman_b";
const FAST_UNIT_TEMPLATE = "units/athen/cavalry_swordsman_b";

const ATTACKER = 2;

var QuickSpawn = function(x, z, template, owner = 1)
{
	let ent = Engine.AddEntity(template);

	let cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpEntOwnership)
		cmpEntOwnership.SetOwner(owner);

	let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	cmpEntPosition.JumpTo(x, z);
	return ent;
};

var Rotate = function(angle, ent)
{
	let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	cmpEntPosition.SetYRotation(angle);
	return ent;
};

var WalkTo = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": true,
	});
	return ent;
};

var FormationWalkTo = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": true,
		"formation": "special/formations/box"
	});
	return ent;
};

var Patrol = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "patrol",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": true,
	});
	return ent;
};

var Attack = function(target, ent)
{
	let comm = {
		"type": "attack",
		"entities": Array.isArray(ent) ? ent : [ent],
		"target": target,
		"queued": true,
		"force": true,
	};
	ProcessCommand(ATTACKER, comm);
	return ent;
};


var gx;
var gy;
var experiments = {};

var manual_dance = function(attacker, target, dance_distance, att_distance = 50, n_attackers = 1)
{
	return () => {
		let dancer = QuickSpawn(gx, gy, target);
		for (let i = 0; i < 100; ++i)
			WalkTo(gx, gy + dance_distance * (i % 2), true, dancer);

		let attackers = [];
		for (let i = 0; i < n_attackers; ++i)
			attackers.push(Attack(dancer, WalkTo(gx + att_distance, gy + i * 2, true, QuickSpawn(gx + att_distance + i, gy, attacker, ATTACKER), ATTACKER)));

		return [[dancer], attackers];
	};
};

var manual_square_dance = function(attacker, target, dance_distance, att_distance = 50, n_attackers = 1)
{
	return () => {
		let dancer = QuickSpawn(gx, gy, target);
		for (let i = 0; i < 25; ++i)
		{
			WalkTo(gx + dance_distance / 2, gy + dance_distance / 2, true, dancer);
			WalkTo(gx + dance_distance / 2, gy - dance_distance / 2, true, dancer);
			WalkTo(gx - dance_distance / 2, gy - dance_distance / 2, true, dancer);
			WalkTo(gx - dance_distance / 2, gy + dance_distance / 2, true, dancer);
		}

		let attackers = [];
		for (let i = 0; i < n_attackers; ++i)
			attackers.push(Attack(dancer, WalkTo(gx + att_distance, gy + i * 2, true, QuickSpawn(gx + att_distance + i, gy, attacker, ATTACKER), ATTACKER)));

		return [[dancer], attackers];
	};
};

var manual_zigzag_dance = function(attacker, target, dance_distance, att_distance = 50, n_attackers = 1)
{
	return () => {
		let dancer = QuickSpawn(gx, gy, target);
		for (let i = 0; i < 12; ++i)
		{
			WalkTo(gx + dance_distance, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 2, gy - dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 3, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 4, gy - dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 5, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 6, gy - dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 7, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 6, gy - dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 5, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 4, gy - dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 3, gy + dance_distance, true, dancer);
			WalkTo(gx + dance_distance * 2, gy - dance_distance, true, dancer);
		}

		let attackers = [];
		for (let i = 0; i < n_attackers; ++i)
			attackers.push(Attack(dancer, WalkTo(gx + att_distance, gy + i * 2, true, QuickSpawn(gx + att_distance + i, gy, attacker, ATTACKER), ATTACKER)));

		return [[dancer], attackers];
	};
};

var patrol_dance = function(attacker, target, dance_distance, att_distance = 50, n_attackers = 1)
{
	return () => {
		let dancer = QuickSpawn(gx, gy, target);
		Patrol(gx, gy + dance_distance, true, dancer);

		let attackers = [];
		for (let i = 0; i < n_attackers; ++i)
			attackers.push(Attack(dancer, WalkTo(gx + att_distance, gy + i * 2, true, QuickSpawn(gx + att_distance + i, gy, attacker, ATTACKER), ATTACKER)));

		return [[dancer], attackers];
	};
};

var manual_formation_dance = function(attacker, target, dance_distance, att_distance = 50, n_attackers = 1)
{
	return () => {
		let dancers = [];
		for (let x = 0; x < 4; x++)
			for (let z = 0; z < 4; z++)
				dancers.push(QuickSpawn(gx+x, gy+z, target));
		for (let i = 0; i < 100; ++i)
			FormationWalkTo(gx, gy + dance_distance * (i % 2), i != 0, dancers);

		let attackers = [];
		for (let i = 0; i < n_attackers; ++i)
			attackers.push(Attack(dancers[0], WalkTo(gx + att_distance, gy + i * 2, true, QuickSpawn(gx + att_distance + i, gy, attacker, ATTACKER), ATTACKER)));

		return [dancers, attackers.concat(dancers)];
	};
};


experiments.unit_manual_dance_archer = {
	"spawn": manual_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 5)
};

experiments.unit_manual_bad_dance_archer = {
	"spawn": manual_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 20)
};

experiments.unit_manual_dance_multi_archer = {
	"spawn": manual_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 5, 50, 4)
};

experiments.fast_unit_manual_dance_archer = {
	"spawn": manual_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 5)
};

experiments.fast_unit_manual_bad_dance_archer = {
	"spawn": manual_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 20)
};

experiments.unit_manual_dance_jav = {
	"spawn": manual_dance(JAV_TEMPLATE, REG_UNIT_TEMPLATE, 5, 20)
};

experiments.unit_manual_bad_dance_jav = {
	"spawn": manual_dance(JAV_TEMPLATE, REG_UNIT_TEMPLATE, 20, 20)
};

experiments.unit_manual_dance_multi_jav = {
	"spawn": manual_dance(JAV_TEMPLATE, REG_UNIT_TEMPLATE, 5, 20, 4)
};

experiments.fast_unit_manual_dance_jav = {
	"spawn": manual_dance(JAV_TEMPLATE, FAST_UNIT_TEMPLATE, 5, 20)
};

experiments.fast_unit_manual_bad_dance_jav = {
	"spawn": manual_dance(JAV_TEMPLATE, FAST_UNIT_TEMPLATE, 20, 20)
};

experiments.fast_patrol_archer = {
	"spawn": patrol_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 5, 50)
};

experiments.fast_patrol_jav = {
	"spawn": patrol_dance(JAV_TEMPLATE, FAST_UNIT_TEMPLATE, 5, 20)
};

experiments.reg_square_archer = {
	"spawn": manual_square_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 5, 50)
};

experiments.reg_square_archer_multi = {
	"spawn": manual_square_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 5, 50, 5)
};

experiments.fast_square_archer = {
	"spawn": manual_square_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 5, 50)
};

experiments.fast_square_archer_multi = {
	"spawn": manual_square_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 5, 50, 5)
};

experiments.reg_zigzag_archer = {
	"spawn": manual_zigzag_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 3, 50)
};

experiments.reg_zigzag_archer_multi = {
	"spawn": manual_zigzag_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 3, 50, 5)
};

experiments.reg_zigzag_jav = {
	"spawn": manual_zigzag_dance(JAV_TEMPLATE, REG_UNIT_TEMPLATE, 3, 20, 5)
};

experiments.fast_zizag_archer = {
	"spawn": manual_zigzag_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 3, 50)
};

experiments.fast_zizag_archer_multi = {
	"spawn": manual_zigzag_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 3, 50, 5)
};

experiments.formation_dance_slow_archer = {
	"spawn": manual_formation_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 25, 50, 5)
};

experiments.formation_dance_fast_archer = {
	"spawn": manual_formation_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 25, 50, 5)
};

experiments.formation_bad_dance_slow_archer = {
	"spawn": manual_formation_dance(ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, 50, 50, 5)
};

experiments.formation_bad_dance_fast_archer = {
	"spawn": manual_formation_dance(ARCHER_TEMPLATE, FAST_UNIT_TEMPLATE, 50, 50, 5)
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

Trigger.prototype.SetupUnits = function()
{
	warn("Experiment start");
	let start = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime();
	gx = 100;
	gy = 100;
	for (let key in experiments)
	{
		let [dancers, attackers] = experiments[key].spawn();
		let ReportResults = (killed) => {
			warn(`Exp ${key} finished in ${Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() - start}, ` +
				`target was ${killed ? "killed" : "not killed (failure)"}`);
			ProcessCommand(1, {
				"type": "delete-entities",
				"entities": dancers,
				"controlAllUnits": true
			});
			ProcessCommand(2, {
				"type": "delete-entities",
				"entities": attackers,
				"controlAllUnits": true
			});
		};
		// xxtreme hack: hook into UnitAI
		let uai = Engine.QueryInterface(dancers[0], IID_UnitAI);
		let odes = uai.OnDestroy;
		uai.OnDestroy = () => ReportResults(true) && odes();
		uai.FindNewTargets = () => {
			ReportResults(false);
			uai.OnDestroy = odes;
		};

		gx += 70;
		if (gx >= 520)
		{
			gx = 100;
			gy += 70;
		}
	}
};

/**
 * Remove unit spread for the second wave.
 */
Trigger.prototype.RemoveSpread = function()
{
	let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
	cmpModifiersManager.AddModifiers("no_promotion", {
		"Attack/Ranged/Spread": [{ "affects": ["Unit"], "replace": 0 }],
	}, 4); // player 2 is ent 4
};

Trigger.prototype.EndGame = function()
{
	Engine.QueryInterface(3, IID_Player).SetState("defeated", "trigger");
	Engine.QueryInterface(4, IID_Player).SetState("won", "trigger");
};

var cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);

// Reduce player 1 vision range (or patrolling units reacct)
cmpModifiersManager.AddModifiers("no_promotion", {
	"Vision/Range": [{ "affects": ["Unit"], "replace": 5 }],
}, 3); // player 1 is ent 3

// Prevent promotions, messes up things.
cmpModifiersManager.AddModifiers("no_promotion_A", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 3);
cmpModifiersManager.AddModifiers("no_promotion_B", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 4); // player 2 is ent 4

cmpTrigger.DoAfterDelay(3000, "SetupUnits", {});
// Second run
cmpTrigger.DoAfterDelay(150000, "RemoveSpread", {});
cmpTrigger.DoAfterDelay(151000, "SetupUnits", {});

cmpTrigger.DoAfterDelay(300000, "EndGame", {});
